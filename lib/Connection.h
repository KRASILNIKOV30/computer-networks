#pragma once
#include "./FileDesc.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <string>

class Connection
{
public:
    Connection(uint16_t port, std::string const& serverAddrStr)
    {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);

        if (inet_pton(AF_INET, serverAddrStr.c_str(), &addr.sin_addr) <= 0)
        {
            addrinfo hints{};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            addrinfo* result;

            std::string portStr = std::to_string(port);

            const int status = getaddrinfo(serverAddrStr.c_str(), portStr.c_str(), &hints, &result);
            if (status != 0)
            {
                throw std::runtime_error("Invalid address or address not supported: " + std::string(gai_strerror(status)));
            }

            if (result->ai_addrlen >= sizeof(addr))
            {
                addr = *reinterpret_cast<sockaddr_in*>(result->ai_addr);
            }
            else
            {
                freeaddrinfo(result);
                throw std::runtime_error("Invalid address structure");
            }

            freeaddrinfo(result);
        }

        if (connect(m_fd.Get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            throw std::runtime_error("Connection failed");
        }
        std::cout << "Connection created to " << serverAddrStr << ":" << port << std::endl;
    }

    void Send(std::string const& message)
    {
        size_t totalSent = 0;
        while (totalSent < message.size())
        {
            ssize_t bytesSent = send(m_fd.Get(), message.data() + totalSent, message.size() - totalSent, 0);
            if (bytesSent == -1)
            {
                throw std::runtime_error("Failed to send message");
            }
            totalSent += bytesSent;
        }
    }

    std::string Receive()
    {
        char buffer[1024];
        const ssize_t bytesRead = recv(m_fd.Get(), buffer, sizeof(buffer) - 1, 0);
        if (bytesRead == -1)
        {
            throw std::runtime_error("Failed to receive message");
        }
        buffer[bytesRead] = '\0';

        return buffer;
    }

    std::string ReadLine()
    {
        size_t pos;
        while ((pos = m_readBuffer.find('\n')) == std::string::npos)
        {
            char buffer[4096];
            const ssize_t bytesRead = recv(m_fd.Get(), buffer, sizeof(buffer), 0);

            if (bytesRead == -1)
            {
                throw std::runtime_error("Failed to receive message (socket error)");
            }
            if (bytesRead == 0)
            {
                throw std::runtime_error("Connection closed by server unexpectedly");
            }

            m_readBuffer.append(buffer, static_cast<size_t>(bytesRead));
        }

        std::string line = m_readBuffer.substr(0, pos + 1);
        m_readBuffer.erase(0, pos + 1);

        return line;
    }

    ~Connection()
    {
        std::cout << "Connection closed" << std::endl;
    }

private:
    FileDesc m_fd{ socket(AF_INET, SOCK_STREAM, /*protocol*/ 0) };
    std::string m_readBuffer;
};