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

#include "Mqtt.h"

#include <iot/mqtt/MqttContext.h>
#include <iot/mqtt/Topic.h>
#include <iot/mqtt/packets/Connack.h>
#include <log/Logger.h>

//

#include <list>
#include <map>
#include <nlohmann/json.hpp>

// IWYU pragma: no_include <nlohmann/detail/json_pointer.hpp>

namespace mqtt::mqttintegrator::lib {

    Mqtt::Mqtt(const nlohmann::json& connectionJson, const nlohmann::json& mappingJson)
        : mqtt::lib::MqttMapper(mappingJson)
        , connectionJson(connectionJson)
        , keepAlive(connectionJson["keep_alive"])
        , clientId(connectionJson["client_id"])
        , cleanSession(connectionJson["clean_session"])
        , willTopic(connectionJson["will_topic"])
        , willMessage(connectionJson["will_message"])
        , willQoS(connectionJson["will_qos"])
        , willRetain(connectionJson["will_retain"])
        , username(connectionJson["username"])
        , password(connectionJson["password"]) {
        LOG(TRACE) << "Keep Alive: " << keepAlive;
        LOG(TRACE) << "Client Id: " << clientId;
        LOG(TRACE) << "Clean Session: " << cleanSession;
        LOG(TRACE) << "Will Topic: " << willTopic;
        LOG(TRACE) << "Will Message: " << willMessage;
        LOG(TRACE) << "Will QoS: " << static_cast<uint16_t>(willQoS);
        LOG(TRACE) << "Will Retain " << willRetain;
        LOG(TRACE) << "Username: " << username;
        LOG(TRACE) << "Password: " << password;
    }

    Mqtt::~Mqtt() {
        pingTimer.cancel();
    }

    void Mqtt::onConnected() {
        VLOG(0) << "On Connected";

        sendConnect(keepAlive, clientId, cleanSession, willTopic, willMessage, willQoS, willRetain, username, password);

        mqttContext->setKeepAlive(keepAlive * 1.5);

        pingTimer = core::timer::Timer::intervalTimer(
            [this](void) -> void {
                sendPingreq();
            },
            keepAlive);
    }

    void Mqtt::onExit() {
        VLOG(0) << "On Exit";
        sendDisconnect();
    }

    void Mqtt::onConnack(iot::mqtt::packets::Connack& connack) {
        if (connack.getReturnCode() == 0 && !connack.getSessionPresent()) {
            sendPublish(getPacketIdentifier(), "snode.c/_cfg_/connection", connectionJson.dump(), 0, true);
            sendPublish(getPacketIdentifier(), "snode.c/_cfg_/mapping", mqtt::lib::MqttMapper::dump(), 0, true);

            std::list<iot::mqtt::Topic> topicList = MqttMapper::extractTopics();

            for (const iot::mqtt::Topic& topic : topicList) {
                LOG(INFO) << "Subscribe Topic: " << topic.getName() << ", qoS: " << static_cast<uint16_t>(topic.getQoS());
            }

            sendSubscribe(getPacketIdentifier(), topicList);
        }
    }

    void Mqtt::onPublish(iot::mqtt::packets::Publish& publish) {
        publishMappings(publish);
    }

    void Mqtt::publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        sendPublish(getPacketIdentifier(), topic, message, qoS, retain);
    }

} // namespace mqtt::mqttintegrator::lib
