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

#include "SocketContext.h" // IWYU pragma: export

#include "iot/mqtt/server/broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace apps::mqttbroker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection,
                                 const std::shared_ptr<iot::mqtt::server::broker::Broker>& broker,
                                 const nlohmann::json& jsonMapping)
        : iot::mqtt::server::SocketContext(socketConnection, broker)
        , apps::mqttbroker::lib::MqttMapper(jsonMapping) {
    }

    void SocketContext::onPublish(iot::mqtt::packets::Publish& publish) {
        publishMappings(publish);
    }

    void SocketContext::publishMappingMatch(const std::string& topic, const std::string& message, uint8_t qoS) {
        broker->publish(topic, message, qoS);
    }

    /*
        iotempower/cfg/test01/ip 192.168.12.183
        iotempower/cfg/test02/ip 192.168.12.132
        iotempower/binary_sensor/test01/button1/config
        {
            "name": "test01 button1",
            "state_topic": "test01/button1",
            "payload_on": "released",
            "payload_off": "pressed"
        },
        iotempower/switch/test02/onboard/config
        {
            "name": "test02 onboard",
            "state_topic": "test02/onboard",
            "state_on": "on",
            "state_off": "off",
            "command_topic": "test02/onboard/set",
            "payload_on": "on",
            "payload_off": "off"
        }

    {
       "binary_sensor" : [{
            "name": "test01 button1",
            "state_topic": "test01/button1",
            "payload_on": "released",
            "payload_off": "pressed"
        }],
        "switch" : [{
            "name": "test02 onboard",
            "state_topic": "test02/onboard",
            "state_on": "on",
            "state_off": "off",
            "command_topic": "test02/onboard/set",
            "payload_on": "on",
            "payload_off": "off"
        }]
    }

    {
        "iotempower" : {
            "test01" : {
                "button1" : {
                    "qos" : 0,
                    "payload" : {
                        "type" : "binary_sensor",
                        "pressed" : {
                            "command_topic" : "test02/onboard/set",
                            "state" : "on"
                        },
                        "released" : {
                            "command_topic" : "test02/onboard/set",
                            "state" : "off"
                        }
                    }
                }
            }
        }
    }

    */

} // namespace apps::mqttbroker
