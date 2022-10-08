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

#ifndef APPS_MQTT_SERVER_TOPICTREE_H
#define APPS_MQTT_SERVER_TOPICTREE_H

namespace mqtt::broker {
    class Broker;
} // namespace mqtt::broker

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <map>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    class RetainTree {
    public:
        explicit RetainTree(mqtt::broker::Broker* broker);

        void retain(const std::string& fullTopicName, const std::string& message, uint8_t qosLevel);
        void publish(std::string fullTopicName, const std::string& clientId, uint8_t clientQoSLevel);

    private:
        class RetainTreeNode {
        public:
            explicit RetainTreeNode(mqtt::broker::Broker* broker);

            bool retain(const std::string& fullTopicName,
                        const std::string& message,
                        uint8_t qoSLevel,
                        std::string remainingTopicName,
                        bool leafFound);
            void publish(const std::string& clientId, uint8_t clientQoSLevel, std::string remainingTopicName, bool leafFound);
            void publish(const std::string& clientId, uint8_t clientQoSLevel);

        private:
            std::string fullTopicName = "";
            std::string message = "";
            uint8_t qoSLevel = 0;

            std::map<std::string, RetainTreeNode> topicTreeNodes;

            mqtt::broker::Broker* broker;
        };

        RetainTreeNode head;
    };

} // namespace mqtt::broker

#endif // APPS_MQTT_SERVER_TOPICTREE_H
