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

#include "mqttbroker/lib/MqttModel.h"

#include <iot/mqtt/packets/Publish.h>
#include <iot/mqtt/server/broker/Broker.h>

namespace mqtt::mqttbroker::lib {

    Mqtt::Mqtt(const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker, const nlohmann::json& mappingJson)
        : iot::mqtt::server::Mqtt(broker)
        , mqtt::lib::MqttMapper(mappingJson) {
    }

    void Mqtt::onConnect(const iot::mqtt::packets::Connect& connect) {
        MqttModel::instance().addConnectedClient(this, connect);
    }

    void Mqtt::onPublish(const iot::mqtt::packets::Publish& publish) {
        publishMappings(publish);
    }

    void Mqtt::onDisconnected() {
        MqttModel::instance().delDisconnectedClient(this);
    }

    void Mqtt::publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) {
        broker->publish(topic, message, qoS);

        if (retain) {
            broker->retainMessage(topic, message, qoS);
        }

        publishMappings(iot::mqtt::packets::Publish(getPacketIdentifier(), topic, message, qoS, retain, false));
    }

} // namespace mqtt::mqttbroker::lib
