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

#ifndef MQTT_SERVER_TOPICTREE_H
#define MQTT_SERVER_TOPICTREE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    class TopicTree {
    public:
        TopicTree(const std::string& fullTopicName, const std::string& value);

        void publish(const std::string& fullTopicName, const std::string& value);

        void traverse();

    private:
        void publish(const std::string& fullTopicName, std::string remainingTopicName, const std::string& value);

        void traverse(unsigned long level);

        std::string fullName;
        std::string value;

        std::map<std::string, TopicTree> topicTree;
    };

} // namespace mqtt::broker

#endif // MQTT_SERVER_TOPICTREE_H
