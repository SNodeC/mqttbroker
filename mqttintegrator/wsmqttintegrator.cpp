/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <core/SNodeC.h>
#include <core/timer/Timer.h>
#include <log/Logger.h>
#include <utils/Config.h>
#include <web/http/client/Request.h>
#include <web/http/client/Response.h>
#include <web/http/legacy/in/Client.h>

//

#include <cstdlib>

template <typename Client>
void doConnect(Client& client, const std::function<void()>& stopTimer = nullptr) {
    client.connect([stopTimer](const typename Client::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum != 0) {
            PLOG(ERROR) << "Connecting to " << socketAddress.toString();
        } else {
            VLOG(0) << "MqttIntegrator connected to " << socketAddress.toString();

            if (stopTimer) {
                stopTimer();
            }
        }
    });
}

int main(int argc, char* argv[]) {
    std::string mappingFilePath;
    utils::Config::add_option("--mqtt-mapping-file", mappingFilePath, "MQTT mapping file (json format) for integration", true, "[path]");

    std::string sessionStore;
    utils::Config::add_option("--mqtt-session-store", sessionStore, "Path to file for the persistent session store", false, "[path]");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_MAPPING_FILE", mappingFilePath.data(), 0);
    setenv("MQTT_SESSION_STORE", sessionStore.data(), 0);

    using WsMqttLegacyIntegrator = web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>;

    using WsMqttLegacyIntegratorConnection = WsMqttLegacyIntegrator::SocketConnection;

    WsMqttLegacyIntegrator legacyClient(
        "legacy",
        [](web::http::client::Request& request) -> void {
            VLOG(0) << "OnRequestBegin";

            request.set("Sec-WebSocket-Protocol", "mqtt");

            request.upgrade("/ws/", "websocket");
        },
        [](web::http::client::Request& request, web::http::client::Response& response) -> void {
            VLOG(0) << "OnResponse";

            response.upgrade(request);
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << "OnResponseError";
            VLOG(0) << "     Status: " << status;
            VLOG(0) << "     Reason: " << reason;
        });

    legacyClient.onDisconnect([&legacyClient](WsMqttLegacyIntegratorConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect";

        VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
        VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

        core::timer::Timer timer = core::timer::Timer::intervalTimer(
            [&legacyClient](const std::function<void()>& stop) -> void {
                doConnect(legacyClient, stop);
            },
            1);
    });

    if (!mappingFilePath.empty()) {
        doConnect(legacyClient);
    }

    return core::SNodeC::start();
}
