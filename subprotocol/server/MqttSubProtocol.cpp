/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "MqttSubProtocol.h"

#include "mqttwebfrontend/Mqtt.h"

#include <web/websocket/SubProtocolContext.h> // for SubProtocolContext
#include <web/websocket/server/SubProtocol.h> // for SubProtocol

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <algorithm>
#include <iomanip>
#include <ostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define PING_INTERVAL 60
#define MAX_FLYING_PINGS 3

namespace web::websocket::subprotocol::echo::server {

    MqttSubProtocol::MqttSubProtocol(SubProtocolContext* subProtocolContext,
                                     const std::string& name,
                                     const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                                     const nlohmann::json& mappingJson)
        : web::websocket::server::SubProtocol(subProtocolContext, name, PING_INTERVAL, MAX_FLYING_PINGS)
        , iot::mqtt::MqttContext(new apps::mqttbroker::webfrontend::Mqtt(broker, mappingJson)) {
        iot::mqtt::MqttContext::setSocketConnection(this->getSocketContextUpgrade()->getSocketConnection());
    }

    MqttSubProtocol::~MqttSubProtocol() {
        keepAliveTimer.cancel();
    }

    std::size_t MqttSubProtocol::receive(char* junk, std::size_t junklen) {
        std::size_t maxReturn = std::min(junklen, size);

        std::copy(buffer.data() + cursor, buffer.data() + cursor + maxReturn, junk);

        cursor += maxReturn;
        size -= maxReturn;

        if (size > 0) {
            // publish Event
        } else {
            buffer.clear();
            cursor = 0;
            size = 0;
        }

        return maxReturn;
    }

    void MqttSubProtocol::send(const char* junk, std::size_t junklen) {
        sendMessage(junk, junklen);
    }

    void MqttSubProtocol::setKeepAlive(const utils::Timeval& timeout) {
        keepAliveTimer = core::timer::Timer::singleshotTimer(
            [this]() -> void {
                sendClose();
            },
            timeout);
    }

    void MqttSubProtocol::end([[maybe_unused]] bool fatal) {
        sendClose();
    }

    void MqttSubProtocol::kill() {
        sendClose();
    }

    void MqttSubProtocol::onConnected() {
        VLOG(0) << "Mqtt connected:";
    }

    void MqttSubProtocol::onMessageStart([[maybe_unused]] int opCode) {
        if (opCode != 2) {
            this->end(true);
        }
    }

    void MqttSubProtocol::onMessageData(const char* junk, std::size_t junkLen) {
        data += std::string(junk, junkLen);
    }

    void MqttSubProtocol::onMessageEnd() {
        std::stringstream ss;

        ss << "WS-Message: ";
        unsigned long i = 0;
        for (char ch : data) {
            if (i != 0 && i % 8 == 0 && i != data.size()) {
                ss << std::endl;
                ss << "                                            ";
            }
            ++i;
            ss << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint16_t>(static_cast<uint8_t>(ch))
               << " "; // << " | ";
        }
        VLOG(0) << ss.str();

        buffer.insert(buffer.end(), data.begin(), data.end());
        size += data.size();
        data.clear();

        if (buffer.size() > 0) {
            iot::mqtt::MqttContext::onReceiveFromPeer();
        }

        keepAliveTimer.restart();
    }

    void MqttSubProtocol::onMessageError(uint16_t errnum) {
        VLOG(0) << "Message error: " << errnum;
    }

    void MqttSubProtocol::onDisconnected() {
        VLOG(0) << "MQTT disconnected:";
    }

    void MqttSubProtocol::onExit() {
        VLOG(0) << "MQTT exit:";
    }

} // namespace web::websocket::subprotocol::echo::server
