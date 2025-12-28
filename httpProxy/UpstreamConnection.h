#pragma once
#include "../lib/FileDesc.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <vector>
#include <cstring>

class UpstreamConnection
{
public:
    UpstreamConnection(const std::string& host, int port)
    {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        addrinfo* result;
        std::string portStr = std::to_string(port);
        const int status = getaddrinfo(host.c_str(), portStr.c_str(), &hints, &result);

        if (status != 0)
        {
            throw std::runtime_error("DNS resolution failed for " + host + ": " + gai_strerror(status));
        }

        bool connected = false;
        for (addrinfo* rp = result; rp != nullptr; rp = rp->ai_next) {
            int sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (sock == -1) continue;

            m_fd = FileDesc(sock);
            if (connect(m_fd.Get(), rp->ai_addr, rp->ai_addrlen) != -1) {
                connected = true;
                break;
            }
            m_fd.Close();
        }

        freeaddrinfo(result);

        if (!connected)
        {
            throw std::runtime_error("Connection to " + host + " failed");
        }
    }

    void Send(const std::string& message)
    {
        size_t totalSent = 0;
        while (totalSent < message.size()) {
            ssize_t sent = send(m_fd.Get(), message.data() + totalSent, message.size() - totalSent, 0);
            if (sent == -1) throw std::runtime_error("Failed to send request");
            totalSent += sent;
        }
    }

    size_t Receive(char* buffer, size_t size)
    {
        ssize_t bytesRead = recv(m_fd.Get(), buffer, size, 0);
        if (bytesRead == -1)
        {
            throw std::runtime_error("Failed to receive data");
        }
        return static_cast<size_t>(bytesRead);
    }

private:
    FileDesc m_fd;
};