#pragma once
#include "./FileDesc.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>

class Connection
{
public:
	Connection(sockaddr_in addr, std::string const& serverAddrStr)
	{
		// Пробуем сначала как IPv4 адрес
		if (inet_pton(AF_INET, serverAddrStr.c_str(), &addr.sin_addr) <= 0)
		{
			// Если не IPv4 адрес, пробуем как доменное имя
			addrinfo hints{};
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			
			addrinfo* result;
			const int status = getaddrinfo(serverAddrStr.c_str(), nullptr, &hints, &result);
			if (status != 0)
			{
				throw std::runtime_error("Invalid address or address not supported: " + std::string(gai_strerror(status)));
			}
			
			// Берем первый результат
			if (result->ai_addrlen >= sizeof(addr))
			{
				addr = *reinterpret_cast<sockaddr_in*>(result->ai_addr);
			}
			else
			{
				throw std::runtime_error("Invalid address structure");
			}
			
			freeaddrinfo(result);
		}
		if (connect(m_fd.Get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
		{
			throw std::runtime_error("Connection failed");
		}
        std::cout << "Connection created" << std::endl;
	}

	void Send(std::string const& message)
	{
		// передавать длину сообщения
		if (send(m_fd.Get(), message.data(), message.size(), 0) == -1)
		{
			throw std::runtime_error("Failed to send message");
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

    ~Connection()
    {
        std::cout << "Connection closed" << std::endl;
    }

private:
	FileDesc m_fd{ socket(AF_INET, SOCK_STREAM, /*protocol*/ 0) };
};