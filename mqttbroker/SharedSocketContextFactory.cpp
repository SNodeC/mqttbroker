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

#include "SharedSocketContextFactory.h" // IWYU pragma: export

#include "Mqtt.h" // IWYU pragma: export
#include "lib/JsonMappingReader.h"

#include <iot/mqtt/SocketContext.h>
#include <iot/mqtt/server/SharedSocketContextFactory.hpp>

//

#include <cstdlib>

namespace mqttbroker::broker {

    SharedSocketContextFactory::SharedSocketContextFactory() {
        char* mappingFile = getenv("MQTT_MAPPING_FILE");

        if (mappingFile != nullptr) {
            nlohmann::json mappingJson = mqttbroker::lib::JsonMappingReader::readMappingFromFile(mappingFile);

            if (!mappingJson.empty()) {
                jsonMapping = mappingJson["mappings"];
            }
        }
    }

    core::socket::SocketContext* SharedSocketContextFactory::create(core::socket::SocketConnection* socketConnection,
                                                                    std::shared_ptr<iot::mqtt::server::broker::Broker>& broker) {
        return new iot::mqtt::SocketContext(socketConnection, new mqttbroker::broker::lib::Mqtt(broker, jsonMapping));
    }

} // namespace mqttbroker::broker
