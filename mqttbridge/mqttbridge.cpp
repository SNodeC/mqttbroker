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

#include "SocketContextFactory.h"
#include "lib/BridgeStore.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <core/SNodeC.h>
//
#include <log/Logger.h>
#include <utils/Config.h>
//
#include <utils/CLI11.hpp>
//

/*
// This can be and is done in CMakeLists.txt
// =========================================
#if !defined(MQTTBRIDGE_IN_STREAM_LEGACY)
#define MQTTBRIDGE_IN_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_IN6_STREAM_LEGACY)
#define MQTTBRIDGE_IN6_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_L2_STREAM_LEGACY)
#define MQTTBRIDGE_L2_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_RC_STREAM_LEGACY)
#define MQTTBRIDGE_RC_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_UN_STREAM_LEGACY)
#define MQTTBRIDGE_UN_STREAM_LEGACY
#endif
#if !defined(MQTTBRIDGE_IN_STREAM_TLS)
#define MQTTBRIDGE_IN_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_IN6_STREAM_TLS)
#define MQTTBRIDGE_IN6_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_L2_STREAM_TLS)
#define MQTTBRIDGE_L2_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_RC_STREAM_TLS)
#define MQTTBRIDGE_RC_STREAM_TLS
#endif
#if !defined(MQTTBRIDGE_UN_STREAM_TLS)
#define MQTTBRIDGE_UN_STREAM_TLS
#endif

#if !defined(MQTTBRIDGE_IN_WEBSOCKET_LEGACY)
#define MQTTBRIDGE_IN_WEBSOCKET_LEGACY
#endif
#if !defined(MQTTBRIDGE_IN6_WEBSOCKET_LEGACY)
#define MQTTBRIDGE_IN6_WEBSOCKET_LEGACY
#endif
#if !defined(MQTTBRIDGE_IN_WEBSOCKET_TLS)
#define MQTTBRIDGE_IN_WEBSOCKET_TLS
#endif
#if !defined(MQTTBRIDGE_IN6_WEBSOCKET_TLS)
#define MQTTBRIDGE_IN6_WEBSOCKET_TLS
#endif
*/

// Select necessary include files
// ==============================
#if defined(MQTTBRIDGE_IN_STREAM_LEGACY)
#include <net/in/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_IN_STREAM_TLS)
#include <net/in/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_IN6_STREAM_LEGACY)
#include <net/in6/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_IN6_STREAM_TLS)
#include <net/in6/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_L2_STREAM_LEGACY)
#include <net/l2/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_L2_STREAM_TLS)
#include <net/l2/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_RC_STREAM_LEGACY)
#include <net/rc/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_RC_STREAM_TLS)
#include <net/rc/stream/tls/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_UN_STREAM_LEGACY)
#include <net/un/stream/legacy/SocketClient.h>
#endif
#if defined(MQTTBRIDGE_UN_STREAM_TLS)
#include <net/un/stream/tls/SocketClient.h>
#endif

#if defined(MQTTBRIDGE_IN_WEBSOCKET_LEGACY)
#include <web/http/legacy/in/Client.h>
#endif
#if defined(MQTTBRIDGE_IN_WEBSOCKET_TLS)
#include <web/http/tls/in/Client.h>
#endif
#if defined(MQTTBRIDGE_IN6_WEBSOCKET_LEGACY)
#include <web/http/legacy/in6/Client.h>
#endif
#if defined(MQTTBRIDGE_IN6_WEBSOCKET_TLS)
#include <web/http/tls/in6/Client.h>
#endif

#include <list>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#endif

static void
reportState(const std::string& instanceName, const core::socket::SocketAddress& socketAddress, const core::socket::State& state) {
    switch (state) {
        case core::socket::State::OK:
            VLOG(1) << instanceName << ": connected to '" << socketAddress.toString() << "'";
            break;
        case core::socket::State::DISABLED:
            VLOG(1) << instanceName << ": disabled";
            break;
        case core::socket::State::ERROR:
            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
        case core::socket::State::FATAL:
            LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
            break;
    }
}

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>, // cppcheck-suppress syntaxError
          typename SocketAddress = typename Client::SocketAddress,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>>
void startClient(const std::string& instanceName,
                 const std::function<void(typename Client::Config&)>& configurator,
                 SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    const Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    configurator(client.getConfig());

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

template <template <typename, typename...> typename SocketClient,
          typename SocketContextFactory,
          typename... SocketContextFactoryArgs,
          typename Client = SocketClient<SocketContextFactory, SocketContextFactoryArgs&&...>,
          typename SocketAddress = typename Client::SocketAddress,
          typename = std::enable_if_t<std::is_base_of_v<core::socket::stream::SocketContextFactory, SocketContextFactory>>,
          typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                              typename SocketClient<SocketContextFactory>::Config&>>>
void startClient(const std::string& instanceName, SocketContextFactoryArgs&&... socketContextFactoryArgs) {
    Client client(instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);

    client.connect([instanceName](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(instanceName, socketAddress, state);
    });
}

template <typename HttpClient>
void startClient(const std::string& name, const std::function<void(typename HttpClient::Config&)>& configurator) {
    using SocketAddress = typename HttpClient::SocketAddress;

    const HttpClient httpClient(
        name,
        [](const std::shared_ptr<web::http::client::Request>& req) -> void {
            req->set("Sec-WebSocket-Protocol", "mqtt");
            req->upgrade(
                "/ws/",
                "websocket",
                [](const std::shared_ptr<web::http::client::Request>& req,
                   const std::shared_ptr<web::http::client::Response>& res) -> void {
                    req->upgrade(
                        res, [subProtocolsRequested = req->header("Upgrade"), subProtocol = res->headers["upgrade"]](bool success) -> void {
                            if (success) {
                                VLOG(1) << "Upgrade to subprotocol '" << subProtocol
                                        << "' successful: Requested: " << subProtocolsRequested;
                            } else {
                                VLOG(1) << "Upgrade to subprotocol '" << subProtocol << "' failed: requested: " << subProtocolsRequested;
                            }
                        });
                },
                [](const std::shared_ptr<web::http::client::Request>& req, const std::string& reason) -> void {
                    VLOG(1) << "Upgrade to subprotocols '" << req->header("Upgrade") << "' failed with response parse error: " << reason;
                });
        },
        []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) -> void {
            VLOG(0) << "Session ended";
        });

    configurator(httpClient.getConfig());

    httpClient.connect([name](const SocketAddress& socketAddress, const core::socket::State& state) -> void {
        reportState(name, socketAddress, state);
    });
}

int main(int argc, char* argv[]) {
#if defined(LINK_WEBSOCKET_STATIC) || defined(LINK_SUBPROTOCOL_STATIC)
    web::websocket::client::SocketContextUpgradeFactory::link();
#endif

#ifdef LINK_SUBPROTOCOL_STATIC
    web::websocket::client::SubProtocolFactorySelector::link("mqtt", mqttClientSubProtocolFactory);
#endif

    CLI::App* bridgeApp = utils::Config::addInstance("bridge", "Configuration for Application mqttbridge", "MQTT-Bridge");
    utils::Config::required(bridgeApp);
    utils::Config::addStandardFlags(bridgeApp);
    utils::Config::addHelp(bridgeApp);

    std::string bridgeDefinitionFile = "<REQUIRED>";
    bridgeApp->needs(bridgeApp->add_option("--definition", bridgeDefinitionFile, "MQTT bridge definition file (JSON format)")
                         ->capture_default_str()
                         ->group(bridgeApp->get_formatter()->get_label("Persistent Options"))
                         ->type_name("[path]")
                         ->configurable()
                         ->required());

    core::SNodeC::init(argc, argv);

    if (bridgeDefinitionFile != "<REQUIRED>") {
        if (mqtt::bridge::lib::BridgeStore::instance().loadAndValidate(bridgeDefinitionFile)) {
            for (const auto& [instanceName, broker] : mqtt::bridge::lib::BridgeStore::instance().getBrokers()) {
                if (!broker.getInstanceName().empty()) {
                    VLOG(1) << "  Creating Broker instance '" << instanceName << "' of Bridge '" << broker.getBridge().getName() << "'";
                    VLOG(1) << "    Broker client id: " << broker.getClientId();
                    VLOG(1) << "    Transport: " << broker.getTransport();
                    VLOG(1) << "    Protocol: " << broker.getProtocol();
                    VLOG(1) << "    Encryption: " << broker.getEncryption();

                    VLOG(1) << "    Topics:";
                    const std::list<iot::mqtt::Topic>& topics = broker.getTopics();
                    for (const iot::mqtt::Topic& topic : topics) {
                        VLOG(1) << "      " << topic.getName() << ":" << static_cast<uint16_t>(topic.getQoS());
                    }

                    const std::string& transport = broker.getTransport();
                    const std::string& protocol = broker.getProtocol();
                    const std::string& encryption = broker.getEncryption();

                    if (transport == "stream") {
                        if (protocol == "in") {
                            if (encryption == "legacy") {
#if defined(MQTTBRIDGE_IN_STREAM_LEGACY)
                                startClient<net::in::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_IN_STREAM_LEGACY
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN_STREAM_LEGACY
                            } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_IN_STREAM_TLS)
                                startClient<net::in::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_IN_STREAM_TLS
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN_STREAM_TLS
                            }
                        } else if (protocol == "in6") {
                            if (encryption == "legacy") {
#if defined(MQTTBRIDGE_IN6_STREAM_LEGACY)
                                startClient<net::in6::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_IN6_STREAM_LEGACY
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN6_STREAM_LEGACY
                            } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_IN6_STREAM_TLS)
                                startClient<net::in6::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_IN6_STREAM_TLS
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN6_STREAM_TLS
                            }
                        } else if (protocol == "l2") {
                            if (encryption == "legacy") {
#if defined(MQTTBRIDGE_L2_STREAM_LEGACY)
                                startClient<net::l2::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_L2_STREAM_LEGACY
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_L2_STREAM_LEGACY
                            } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_L2_STREAM_TLS)
                                startClient<net::l2::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_L2_STREAM_TLS
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_L2_STREAM_TLS
                            }
                        } else if (protocol == "rc") {
                            if (encryption == "legacy") {
#if defined(MQTTBRIDGE_RC_STREAM_LEGACY)
                                startClient<net::rc::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_RC_STREAM_LEGACY
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_RC_STREAM_LEGACY
                            } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_RC_STREAM_TLS)
                                startClient<net::rc::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_RC_STREAM_TLS
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_RC_STREAM_TLS
                            }
                        } else if (protocol == "un") {
                            if (encryption == "legacy") {
#if defined(MQTTBRIDGE_UN_STREAM_LEGACY)
                                startClient<net::un::stream::legacy::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_UN_STREAM_LEGACY
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_UN_STREAM_LEGACY
                            } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_UN_STREAM_TLS)
                                startClient<net::un::stream::tls::SocketClient, mqtt::bridge::SocketContextFactory>(
                                    instanceName,
                                    [](auto& config) -> void {
                                        config.setRetry();
                                        config.setRetryBase(1);
                                        config.setReconnect();
                                    },
                                    broker);
#else  // MQTTBRIDGE_UN_STREAM_TLS
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_UN_STREAM_TLS
                            }
                        }
                    } else if (transport == "websocket") {
                        if (protocol == "in") {
                            if (encryption == "legacy") {
#if defined(MQTTBRIDGE_IN_WEBSOCKET_LEGACY)
                                startClient<web::http::legacy::in::Client>(broker.getInstanceName(), [](auto& config) -> void {
                                    config.Remote::setPort(8080);

                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                });
#else  // MQTTBRIDGE_IN_WEBSOCKET_LEGACY
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN_WEBSOCKET_LEGACY
                            } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_IN_WEBSOCKET_TLS)
                                startClient<web::http::tls::in::Client>(broker.getInstanceName(), [](auto& config) -> void {
                                    config.Remote::setPort(8088);

                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                });
#else  // MQTTBRIDGE_IN_WEBSOCKET_TLS
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN_WEBSOCKET_TLS
                            }
                        } else if (protocol == "in6") {
                            if (encryption == "legacy") {
#if defined(MQTTBRIDGE_IN6_WEBSOCKET_LEGACY)
                                startClient<web::http::legacy::in6::Client>(broker.getInstanceName(), [](auto& config) -> void {
                                    config.Remote::setPort(8080);

                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                });
#else  // MQTTBRIDGE_IN6_WEBSOCKET_LEGACY
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN6_WEBSOCKET_LEGACY
                            } else if (encryption == "tls") {
#if defined(MQTTBRIDGE_IN6_WEBSOCKET_TLS)
                                startClient<web::http::tls::in6::Client>(broker.getInstanceName(), [](auto& config) -> void {
                                    config.Remote::setPort(8088);

                                    config.setRetry();
                                    config.setRetryBase(1);
                                    config.setReconnect();
                                });
#else  // MQTTBRIDGE_IN6_WEBSOCKET_TLS
                                VLOG(1) << "    Transport '" << transport << "', protocol '" << protocol << "', encryption '" << encryption
                                        << "' not supported.";
#endif // MQTTBRIDGE_IN6_WEBSOCKET_TLS
                            }
                        }
                    } else {
                        VLOG(1) << "    Transport '" << transport << "' not supported.";
                    }
                } else {
                    VLOG(1) << "Instance not created: Instance name empty.";
                }
            }
        }
    }

    return core::SNodeC::start();
}
