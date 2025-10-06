#pragma once
#include <functional>
#include <netinet/in.h>
#include <system_error>
#include "Socket.h"

class Acceptor
{
public:
	Acceptor(const sockaddr_in& addr, const int queueSize)
	{
		if (bind(m_fd.Get(), reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0)
		{
			throw std::system_error(errno, std::generic_category());
		}
		if (listen(m_fd.Get(), queueSize) != 0)
		{
			throw std::system_error(errno, std::generic_category());
		}
	}

	// Ждёт первое подключение к сокету
	// https://man7.org/linux/man-pages/man2/accept.2.html
	[[nodiscard]] Socket Accept() const
	{
		sockaddr_in clientAddress{};
		socklen_t clientLength = sizeof(clientAddress);
		FileDesc clientFd{ accept(m_fd.Get(), reinterpret_cast<sockaddr*>(&clientAddress), &clientLength) };
		return Socket{ std::move(clientFd) };
	}

private:
	FileDesc m_fd{ socket(AF_INET, SOCK_STREAM, /*protocol*/ 0) };
};