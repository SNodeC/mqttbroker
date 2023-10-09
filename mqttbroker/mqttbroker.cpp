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

#include "lib/Mqtt.h"
#include "lib/MqttModel.h"
#include "lib/SharedSocketContextFactory.h" // IWYU pragma: keep

#include <core/SNodeC.h>
#include <express/legacy/in/WebApp.h>
#include <express/tls/in/WebApp.h>
#include <net/in/stream/legacy/SocketServer.h> // IWYU pragma: keep
#include <net/in/stream/tls/SocketServer.h>    // IWYU pragma: keep
#include <net/un/stream/legacy/SocketServer.h> // IWYU pragma: keep
#include <utils/Config.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdlib>

#endif

namespace iot::mqtt::packets {
    class Connect;
}

express::Router getRouter();
express::Router getRouter() {
    express::Router router;
    router.get("/clients", [] APPLICATION(req, res) {
        const std::map<mqtt::mqttbroker::lib::Mqtt*, iot::mqtt::packets::Connect>& connectionList =
            mqtt::mqttbroker::lib::MqttModel::instance().getConnectedClinets();

        std::string responseString = "<html>"
                                     "  <head>"
                                     "    <title>Response from MqttWebFrontend</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>List of all Connected Clients</h1>"
                                     "    <table>"
                                     "      <tr><th>ClientId</th><th>Locale Address</th><th>Remote Address</th></tr>";

        for (const auto& [mqtt, connectPacket] : connectionList) {
            responseString += "<tr><td>" + mqtt->getClientId() + "</td><td>" + mqtt->getSocketConnection()->getLocalAddress().toString() +
                              "</td><td>" + mqtt->getSocketConnection()->getRemoteAddress().toString() + "</td></tr>";
        }

        responseString += "    </table>"
                          "  </body>"
                          "</html>";

        res.send(responseString);
    });

    router.get("/ws/", [] APPLICATION(req, res) -> void {
        if (!res.upgrade(req)) {
            res.end();
        }
    });

    router.get("/", [] APPLICATION(req, res) -> void {
        if (!res.upgrade(req)) {
            res.end();
        }
    });

    return router;
}

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--mqtt-mapping-file", "MQTT mapping file (json format) for integration", "[path]", "");
    utils::Config::add_string_option("--mqtt-session-store", "Path to file for the persistent session store", "[path]", "");

    core::SNodeC::init(argc, argv);

    setenv("MQTT_SESSION_STORE", utils::Config::get_string_option_value("--mqtt-session-store").data(), 0);

    {
        using MQTTLegacyInServer = net::in::stream::legacy::SocketServer<mqtt::mqttbroker::lib::SharedSocketContextFactory>;
        using MQTTLegacyInSocketAddress = MQTTLegacyInServer::SocketAddress;

        MQTTLegacyInServer mqttLegacyInServer("legacyin");
        mqttLegacyInServer.listen([](const MQTTLegacyInSocketAddress& socketAddress, core::socket::State state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << "legacyin: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << "legacyin: disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << "legacyin: non critical error occurred";
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << "legacyin: critical error occurred";
                    break;
            }
        });

        using MQTTTLSInServer = net::in::stream::tls::SocketServer<mqtt::mqttbroker::lib::SharedSocketContextFactory>;
        using MQTTTLSInSocketAddress = MQTTTLSInServer::SocketAddress;

        MQTTTLSInServer mqttTLSInServer("tlsin");
        mqttTLSInServer.listen([](const MQTTTLSInSocketAddress& socketAddress, core::socket::State state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << "tlsin: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << "tlsin: disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << "tlsin: non critical error occurred";
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << "tlsin: critical error occurred";
                    break;
            }
        });

        using MQTTLegacyUnServer = net::un::stream::legacy::SocketServer<mqtt::mqttbroker::lib::SharedSocketContextFactory>;
        using MQTTTLSUnSocketAddress = MQTTLegacyUnServer::SocketAddress;

        MQTTLegacyUnServer mqttLegacyUnServer("legacyun");
        mqttLegacyUnServer.listen([](const MQTTTLSUnSocketAddress& socketAddress, core::socket::State state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << "legacyun: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << "legacyun: disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << "legacyun: non critical error occurred";
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << "legacyun: critical error occurred";
                    break;
            }
        });

        using MQTTTLSWebView = express::tls::in::WebApp;
        using MQTTTlSWebViewSocketAddress = MQTTTLSWebView::SocketAddress;

        MQTTTLSWebView mqttTLSWebView("mqtttlswebview", getRouter());
        mqttTLSWebView.listen([](const MQTTTlSWebViewSocketAddress& socketAddress, core::socket::State state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << "mqtttlswebview: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << "mqtttlswebview: disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << "mqtttlswebview: non critical error occurred";
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << "mqtttlswebview: critical error occurred";
                    break;
            }
        });

        using MQTTLegacyWebView = express::legacy::in::WebApp;
        using MQTTLegacyWebViewSocketAddress = MQTTLegacyWebView::SocketAddress;

        MQTTLegacyWebView mqttLegacyWebView("mqttlegacywebview", mqttTLSWebView);
        mqttLegacyWebView.listen([](const MQTTLegacyWebViewSocketAddress& socketAddress, core::socket::State state) -> void {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << "mqttlegacywebview: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << "mqttlegacywebview: disabled";
                    break;
                case core::socket::State::ERROR:
                    VLOG(1) << "mqttlegacywebview: non critical error occurred";
                    break;
                case core::socket::State::FATAL:
                    VLOG(1) << "mqttlegacywebview: critical error occurred";
                    break;
            }
        });
    }

    return core::SNodeC::start();
}
