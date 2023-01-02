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

#include "SocketContextFactory.h" // IWYU pragma: keep

#include <core/SNodeC.h>
#include <core/timer/Timer.h>
#include <log/Logger.h>
#include <net/in/stream/tls/SocketClient.h>
#include <utils/Config.h>

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

    core::SNodeC::init(argc, argv);

    setenv("MQTT_MAPPING_FILE", mappingFilePath.data(), 0);

    using InMqttTlsIntegratorClient = net::in::stream::tls::SocketClient<mqtt::mqttintegrator::SocketContextFactory>;
    using TLSInSocketConnection = InMqttTlsIntegratorClient::SocketConnection;

    InMqttTlsIntegratorClient inMqttTlsIntegratorClient("mqtttlsintegrator");

    inMqttTlsIntegratorClient.onDisconnect([&inMqttTlsIntegratorClient](TLSInSocketConnection* socketConnection) -> void {
        VLOG(0) << "OnDisconnect";

        VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
        VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();

        core::timer::Timer timer = core::timer::Timer::intervalTimer(
            [&inMqttTlsIntegratorClient](const std::function<void()>& stop) -> void {
                doConnect(inMqttTlsIntegratorClient, stop);
            },
            1);
    });

    if (!mappingFilePath.empty()) {
        doConnect(inMqttTlsIntegratorClient);
    }

    return core::SNodeC::start();
}
