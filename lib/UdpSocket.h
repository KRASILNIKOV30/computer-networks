#pragma once
#include "FileDesc.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cstring>

class UdpSocket
{
public:
    UdpSocket()
            : m_fd(socket(AF_INET, SOCK_DGRAM, 0))
    {
        if (!m_fd.IsOpen())
        {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void Bind(int port)
    {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(m_fd.Get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        {
            throw std::system_error(errno, std::generic_category());
        }
        std::cout << "UDP Socket bound to port " << port << std::endl;
    }

    void SetRecvTimeout(int seconds)
    {
        timeval tv{};
        tv.tv_sec = seconds;
        tv.tv_usec = 0;

        if (setsockopt(m_fd.Get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            throw std::system_error(errno, std::generic_category());
        }
    }

    void SendTo(const std::string& message, const sockaddr_in& destAddr)
    {
        if (sendto(m_fd.Get(), message.data(), message.size(), 0,
                   reinterpret_cast<const sockaddr*>(&destAddr), sizeof(destAddr)) < 0)
        {
            throw std::system_error(errno, std::generic_category());
        }
    }

    std::string RecvFrom(sockaddr_in& srcAddr)
    {
        char buffer[1024];
        socklen_t addrLen = sizeof(srcAddr);

        ssize_t len = recvfrom(m_fd.Get(), buffer, sizeof(buffer) - 1, 0,
                               reinterpret_cast<sockaddr*>(&srcAddr), &addrLen);

        if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                throw std::runtime_error("Request timed out");
            }
            throw std::system_error(errno, std::generic_category());
        }

        buffer[len] = '\0';
        return std::string(buffer);
    }

private:
    FileDesc m_fd;
};