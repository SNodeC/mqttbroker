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

#ifndef IOT_MQTTBROKER_MQTTBRIDGE_BRIDGE_H
#define IOT_MQTTBROKER_MQTTBRIDGE_BRIDGE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace iot::mqtt {
    class Mqtt;

    namespace packets {
        class Publish;
    }
} // namespace iot::mqtt

#include <cstdint>
#include <list>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace mqtt::bridge::lib {

    class Bridge {
    public:
        Bridge(const std::string& clientId,
               uint16_t keepAlive,
               bool cleanSession,
               const std::string& willTopic,
               const std::string& willMessage,
               uint8_t willQoS,
               bool willRetain,
               const std::string& userName,
               const std::string& passWord);

        ~Bridge() = default;

        void addMqtt(iot::mqtt::Mqtt* mqtt);
        void removeMqtt(iot::mqtt::Mqtt* mqtt);

        void publish(const iot::mqtt::Mqtt* originMqtt, const iot::mqtt::packets::Publish& publish);

        const std::string& getClientId();
        uint16_t getKeepAlive() const;
        bool getCleanSession() const;
        const std::string& getWillTopic() const;
        const std::string& getWillMessage() const;
        uint8_t getWillQoS() const;
        bool getWillRetain() const;
        const std::string& getUsername() const;
        const std::string& getPassword() const;
        const std::list<iot::mqtt::Mqtt*>& getMqttList() const;

    private:
        std::string clientId;
        uint16_t keepAlive;
        bool cleanSession;
        std::string willTopic;
        std::string willMessage;
        uint8_t willQoS;
        bool willRetain;
        std::string username;
        std::string password;

        std::list<iot::mqtt::Mqtt*> mqttList;
    };

} // namespace mqtt::bridge::lib

#endif // IOT_MQTTBROKER_MQTTBRIDGE_BRIDGE_H
