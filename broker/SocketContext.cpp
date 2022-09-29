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

#include "broker/SocketContext.h"

#include "broker/Broker.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Timeval.h" // IWYU pragma: keep

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace mqtt::broker {

    SocketContext::SocketContext(core::socket::SocketConnection* socketConnection, std::shared_ptr<Broker> broker)
        : iot::mqtt::SocketContext(socketConnection)
        , broker(broker) {
    }

    SocketContext::~SocketContext() {
        if (cleanSession) {
            broker->deleteSession(clientId);
        }

        if (willFlag) {
            broker->publish(willTopic, willMessage, willQoS, willRetain);
        }
    }

    void SocketContext::initSession() {
        if (!broker->hasSession(clientId)) {
            sendConnack(level <= MQTT_VERSION_3_1_1 ? MQTT_CONNACK_ACCEPT : MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_NEW);

            broker->newSession(clientId, this);
        } else if (broker->getSocketContext(clientId) == nullptr) {
            if (cleanSession) {
                broker->unsubscribe(clientId);
            }
            sendConnack(level <= MQTT_VERSION_3_1_1 ? MQTT_CONNACK_ACCEPT : MQTT_CONNACK_UNACEPTABLEVERSION, MQTT_SESSION_PRESENT);

            broker->renewSession(clientId, this);
        } else { // Error ClientId already used
            LOG(TRACE) << "ClientID \'" << clientId << "\' already in use ... disconnecting";

            close(); // from base class
        }
    }

    void SocketContext::releaseSession() {
        if (cleanSession && subscribtionCount > 0) {
            broker->unsubscribe(clientId);
        }
        broker->retainSession(clientId);

        willFlag = false;
    }

    void SocketContext::onConnect(const iot::mqtt::packets::Connect& connect) {
        // V-Header
        protocol = connect.getProtocol();
        level = connect.getLevel();
        connectFlags = connect.getFlags();
        keepAlive = connect.getKeepAlive();

        // Payload
        clientId = connect.getClientId();
        if (clientId.empty()) {
            clientId = broker->getRandomClientId();
        }
        willTopic = connect.getWillTopic();
        willMessage = connect.getWillMessage();
        username = connect.getUsername();
        password = connect.getPassword();

        // Derived from flags
        usernameFlag = connect.getUsernameFlag();
        passwordFlag = connect.getPasswordFlag();
        willRetain = connect.getWillRetain();
        willQoS = connect.getWillQoS();
        willFlag = connect.getWillFlag();
        cleanSession = connect.getCleanSession();

        LOG(DEBUG) << "CONNECT";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << connect.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(connect.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(connect.getReserved());
        LOG(DEBUG) << "RemainingLength: " << connect.getRemainingLength();
        LOG(DEBUG) << "Protocol: " << protocol;
        LOG(DEBUG) << "Version: " << static_cast<uint16_t>(level);
        LOG(DEBUG) << "ConnectFlags: " << static_cast<uint16_t>(connectFlags);
        LOG(DEBUG) << "KeepAlive: " << keepAlive;
        if (keepAlive != 0) {
            setTimeout(1.5 * keepAlive);
        } else {
            // Leave the read- and write-timeouts as configured for this SocketContext (default 60 sec)
        }
        LOG(DEBUG) << "ClientID: " << clientId;
        LOG(DEBUG) << "CleanSession: " << cleanSession;
        if (willFlag) {
            LOG(DEBUG) << "WillTopic: " << willTopic;
            LOG(DEBUG) << "WillMessage: " << willMessage;
            LOG(DEBUG) << "WillQoS: " << static_cast<uint16_t>(willQoS);
            LOG(DEBUG) << "WillRetain: " << willRetain;
        }
        if (usernameFlag) {
            LOG(DEBUG) << "Username: " << username;
        }
        if (passwordFlag) {
            LOG(DEBUG) << "Password: " << password;
        }

        initSession();
    }

    void SocketContext::onConnack(const iot::mqtt::packets::Connack& connack) {
        LOG(DEBUG) << "CONNACK";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << connack.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(connack.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(connack.getReserved());
        LOG(DEBUG) << "RemainingLength: " << connack.getRemainingLength();
        LOG(DEBUG) << "Flags: " << static_cast<uint16_t>(connack.getFlags());
        LOG(DEBUG) << "Reason: " << connack.getReason();
    }

    void SocketContext::onPublish(const iot::mqtt::packets::Publish& publish) {
        LOG(DEBUG) << "PUBLISH";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << publish.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(publish.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(publish.getReserved());
        LOG(DEBUG) << "RemainingLength: " << publish.getRemainingLength();
        LOG(DEBUG) << "DUP: " << publish.getDup();
        LOG(DEBUG) << "QoSLevel: " << static_cast<uint16_t>(publish.getQoSLevel());
        LOG(DEBUG) << "Retain: " << publish.getRetain();
        LOG(DEBUG) << "Topic: " << publish.getTopic();
        LOG(DEBUG) << "PacketIdentifier: " << publish.getPacketIdentifier();
        LOG(DEBUG) << "Message: " << publish.getMessage();

        switch (publish.getQoSLevel()) {
            case 1:
                sendPuback(publish.getPacketIdentifier());
                break;
            case 2:
                sendPubrec(publish.getPacketIdentifier());
                break;
            case 3:
                LOG(TRACE) << "Received publish with QoS-Level 3 ... closing";
                close();
                break;
        }

        if (publish.getQoSLevel() < 3) {
            broker->publish(publish.getTopic(), publish.getMessage(), publish.getQoSLevel());
            if (publish.getRetain()) {
                broker->retain(publish.getTopic(), publish.getMessage(), publish.getQoSLevel());
            }
        } else {
            close();
        }
    }

    void SocketContext::onPuback(const iot::mqtt::packets::Puback& puback) {
        LOG(DEBUG) << "PUBACK";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << puback.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(puback.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(puback.getReserved());
        LOG(DEBUG) << "RemainingLength: " << puback.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << puback.getPacketIdentifier();
    }

    void SocketContext::onPubrec(const iot::mqtt::packets::Pubrec& pubrec) {
        LOG(DEBUG) << "PUBREC";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << pubrec.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pubrec.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pubrec.getReserved());
        LOG(DEBUG) << "RemainingLength: " << pubrec.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << pubrec.getPacketIdentifier();

        sendPubrel(pubrec.getPacketIdentifier());
    }

    void SocketContext::onPubrel(const iot::mqtt::packets::Pubrel& pubrel) {
        LOG(DEBUG) << "PUBREL";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << pubrel.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pubrel.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pubrel.getReserved());
        LOG(DEBUG) << "RemainingLength: " << pubrel.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << pubrel.getPacketIdentifier();

        sendPubcomp(pubrel.getPacketIdentifier());
    }

    void SocketContext::onPubcomp(const iot::mqtt::packets::Pubcomp& pubcomp) {
        LOG(DEBUG) << "PUBCOMP";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << pubcomp.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pubcomp.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pubcomp.getReserved());
        LOG(DEBUG) << "RemainingLength: " << pubcomp.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << pubcomp.getPacketIdentifier();
    }

    void SocketContext::onSubscribe(const iot::mqtt::packets::Subscribe& subscribe) {
        LOG(DEBUG) << "SUBSCRIBE";
        LOG(DEBUG) << "=========";
        LOG(DEBUG) << "Error: " << subscribe.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(subscribe.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(subscribe.getReserved());
        LOG(DEBUG) << "RemainingLength: " << subscribe.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << subscribe.getPacketIdentifier();

        std::list<uint8_t> returnCodes;

        for (const iot::mqtt::Topic& topic : subscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic.getName() << ", requestedQoS: " << static_cast<uint16_t>(topic.getRequestedQoS());
            broker->subscribe(topic.getName(), clientId, topic.getRequestedQoS());

            returnCodes.push_back(topic.getRequestedQoS() | 0x00 /* 0x80 */); // QoS + Success
        }

        sendSuback(subscribe.getPacketIdentifier(), returnCodes);

        subscribtionCount++;
    }

    void SocketContext::onSuback(const iot::mqtt::packets::Suback& suback) {
        LOG(DEBUG) << "SUBACK";
        LOG(DEBUG) << "======";
        LOG(DEBUG) << "Error: " << suback.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(suback.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(suback.getReserved());
        LOG(DEBUG) << "RemainingLength: " << suback.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << suback.getPacketIdentifier();

        for (uint8_t returnCode : suback.getReturnCodes()) {
            LOG(DEBUG) << "  Return Code: " << static_cast<uint16_t>(returnCode);
        }
    }

    void SocketContext::onUnsubscribe(const iot::mqtt::packets::Unsubscribe& unsubscribe) {
        LOG(DEBUG) << "UNSUBSCRIBE";
        LOG(DEBUG) << "===========";
        LOG(DEBUG) << "Error: " << unsubscribe.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(unsubscribe.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(unsubscribe.getReserved());
        LOG(DEBUG) << "RemainingLength: " << unsubscribe.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << unsubscribe.getPacketIdentifier();

        for (const std::string& topic : unsubscribe.getTopics()) {
            LOG(DEBUG) << "  Topic: " << topic;
            broker->unsubscribe(topic, clientId);
        }

        sendUnsuback(unsubscribe.getPacketIdentifier());

        subscribtionCount--;
    }

    void SocketContext::onUnsuback(const iot::mqtt::packets::Unsuback& unsuback) {
        LOG(DEBUG) << "UNSUBACK";
        LOG(DEBUG) << "========";
        LOG(DEBUG) << "Error: " << unsuback.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(unsuback.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(unsuback.getReserved());
        LOG(DEBUG) << "RemainingLength: " << unsuback.getRemainingLength();
        LOG(DEBUG) << "PacketIdentifier: " << unsuback.getPacketIdentifier();
    }

    void SocketContext::onPingreq(const iot::mqtt::packets::Pingreq& pingreq) {
        LOG(DEBUG) << "PINGREQ";
        LOG(DEBUG) << "=======";
        LOG(DEBUG) << "Error: " << pingreq.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pingreq.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(pingreq.getReserved());
        LOG(DEBUG) << "RemainingLength: " << pingreq.getRemainingLength();

        sendPingresp();
    }

    void SocketContext::onPingresp(const iot::mqtt::packets::Pingresp& pingresp) {
        LOG(DEBUG) << "PINGRESP";
        LOG(DEBUG) << "========";
        LOG(DEBUG) << "Error: " << pingresp.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(pingresp.getType());
        LOG(DEBUG) << "RemainingLength: " << pingresp.getRemainingLength();
    }

    void SocketContext::onDisconnect(const iot::mqtt::packets::Disconnect& disconnect) {
        LOG(DEBUG) << "DISCONNECT";
        LOG(DEBUG) << "==========";
        LOG(DEBUG) << "Error: " << disconnect.isError();
        LOG(DEBUG) << "Type: " << static_cast<uint16_t>(disconnect.getType());
        LOG(DEBUG) << "Reserved: " << static_cast<uint16_t>(disconnect.getReserved());
        LOG(DEBUG) << "RemainingLength: " << disconnect.getRemainingLength();

        releaseSession();

        shutdown(); // from base class
    }

} // namespace mqtt::broker
