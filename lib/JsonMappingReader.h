/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#ifndef MQTTBROKER_LIB_JSONMAPPINGREADER_H
#define MQTTBROKER_LIB_JSONMAPPINGREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <string>

#endif

namespace mqtt::lib {

    class JsonMappingReader {
    public:
        JsonMappingReader() = delete;

        static nlohmann::json& readMappingFromFile(const std::string& mapFilePath);

    private:
        static nlohmann::json mappingJsonSchema;
        static const std::string mappingJsonSchemaString;

        static std::map<std::string, nlohmann::json> mapFileJsons;
    };

} // namespace mqtt::lib

#endif // MQTTBROKER_LIB_JSONMAPPINGREADER_H
