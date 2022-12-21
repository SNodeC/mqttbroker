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

#include "SubProtocolFactory.h"

#include "lib/JsonMappingReader.h"
#include "mqttbroker/lib/Mqtt.h"

#include <iot/mqtt/server/broker/Broker.h>

//

#include <cstdlib>

// IWYU pragma: no_include  <iot/mqtt/MqttSubProtocol.hpp>

namespace mqtt::mqttbroker::websocket {

    SubprotocolFactory::SubprotocolFactory(const std::string& name)
        : web::websocket::SubProtocolFactory<iot::mqtt::server::SubProtocol>::SubProtocolFactory(name) {
        char* mappingFile = getenv("MQTT_MAPPING_FILE");

        if (mappingFile != nullptr) {
            nlohmann::json mappingJson = mqtt::lib::JsonMappingReader::readMappingFromFile(mappingFile);

            if (!mappingJson.empty()) {
                jsonMapping = mappingJson["mapping"];
            }
        }
    }

    iot::mqtt::server::SubProtocol* SubprotocolFactory::create(web::websocket::SubProtocolContext* subProtocolContext) {
        return new iot::mqtt::server::SubProtocol(
            subProtocolContext,
            getName(),
            new mqtt::mqttbroker::lib::Mqtt(iot::mqtt::server::broker::Broker::instance(SUBSCRIBTION_MAX_QOS), jsonMapping));
    }

} // namespace mqtt::mqttbroker::websocket

#define NAME "mqtt"

extern "C" void* mqttServerSubProtocolFactory() {
    return new mqtt::mqttbroker::websocket::SubprotocolFactory(NAME);
}
