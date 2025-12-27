#pragma once
#include "../../../lib/FileDesc.h"
#include "Packet.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

class RdtSocket {
public:
    RdtSocket() : m_fd(socket(AF_INET, SOCK_DGRAM, 0)) {
        if (!m_fd.IsOpen()) throw std::runtime_error("Socket creation failed");
    }

    void Bind(uint16_t port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(m_fd.Get(), (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            throw std::runtime_error("Bind failed");
        }
    }

    void SetTimeout(int ms) {
        struct timeval tv;
        tv.tv_sec = ms / 1000;
        tv.tv_usec = (ms % 1000) * 1000;
        setsockopt(m_fd.Get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    void SendTo(Packet& packet, const sockaddr_in& dest) {
        auto data = packet.Serialize();
        sendto(m_fd.Get(), data.data(), data.size(), 0, (struct sockaddr*)&dest, sizeof(dest));
    }

    bool RecvFrom(Packet& packet, sockaddr_in* sender = nullptr) {
        std::vector<uint8_t> buffer(MAX_PAYLOAD_SIZE + HEADER_SIZE);
        sockaddr_in tempSender{};
        socklen_t len = sizeof(tempSender);

        ssize_t received = recvfrom(m_fd.Get(), buffer.data(), buffer.size(), 0, (struct sockaddr*)&tempSender, &len);

        if (received < 0) {
            return false;
        }

        buffer.resize(received);
        if (Packet::Deserialize(buffer, packet)) {
            if (sender) *sender = tempSender;
            return true;
        }
        return false;
    }

private:
    FileDesc m_fd;
};