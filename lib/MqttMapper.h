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

#ifndef MQTTBROKER_LIB_MQTTMAPPER_H
#define MQTTBROKER_LIB_MQTTMAPPER_H

namespace iot::mqtt {
    class Topic;
    namespace packets {
        class Publish;
    }
} // namespace iot::mqtt

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wc++98-compat-pedantic")
#pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#if __has_warning("-Wexit-time-destructors")
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#endif
#if __has_warning("-Wglobal-constructors")
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif
#if __has_warning("-Wreserved-macro-identifier")
#pragma GCC diagnostic ignored "-Wreserved-macro-identifier"
#endif
#if __has_warning("-Wswitch-enum")
#pragma GCC diagnostic ignored "-Wswitch-enum"
#endif
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#endif
#endif
#include "inja.hpp"
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif

#include <cstdint>
#include <functional>
#include <list>
#include <nlohmann/json_fwd.hpp> // IWYU pragma: export
#include <string>

#endif

namespace mqtt::lib {

    class MqttMapper {
    public:
        MqttMapper(const nlohmann::json& mappingJson);

        virtual ~MqttMapper();

    protected:
        std::string dump();

        std::list<iot::mqtt::Topic> extractSubscriptions();
        void publishMappings(const iot::mqtt::packets::Publish& publish);

    private:
        virtual void publishMapping(const std::string& topic, const std::string& message, uint8_t qoS, bool retain) = 0;

        static void extractSubscription(const nlohmann::json& json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList);
        static void extractSubscriptions(const nlohmann::json& json, const std::string& topic, std::list<iot::mqtt::Topic>& topicList);

        nlohmann::json findMatchingTopicLevel(const nlohmann::json& topicLevel, const std::string& topic);

        void publishMappedTemplate(const nlohmann::json& templateMapping, nlohmann::json& json, const iot::mqtt::packets::Publish& publish);
        void
        publishMappedTemplates(const nlohmann::json& templateMapping, nlohmann::json& json, const iot::mqtt::packets::Publish& publish);

        void
        publishMappedMessage(const nlohmann::json& staticMapping, const std::string& message, const iot::mqtt::packets::Publish& publish);
        void publishMappedMessage(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish);
        void publishMappedMessages(const nlohmann::json& staticMapping, const iot::mqtt::packets::Publish& publish);

        const nlohmann::json& mappingJson;

        inja::Environment injaEnvironment;
    };

    struct FunctionBase {
        FunctionBase(const std::string& name, int numArgs)
            : name(name)
            , numArgs(numArgs) {
        }

        std::string name;
        int numArgs;
    };

    struct Function : FunctionBase {
        Function(const std::string& name, int numArgs, const std::function<inja::json(inja::Arguments&)>& function)
            : FunctionBase(name, numArgs)
            , function(function) {
        }

        std::function<inja::json(inja::Arguments&)> function;
    };

    struct VoidFunction : FunctionBase {
        VoidFunction(const std::string& name, int numArgs, const std::function<void(inja::Arguments&)>& function)
            : FunctionBase(name, numArgs)
            , function(function) {
        }

        std::function<void(inja::Arguments&)> function;
    };

} // namespace mqtt::lib

#endif // MQTTBROKER_LIB_MQTTMAPPER_H
