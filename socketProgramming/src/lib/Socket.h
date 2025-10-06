#pragma once
#include "FileDesc.h"
#include <sys/socket.h>

class Socket
{
public:
	explicit Socket(FileDesc fd)
		: m_fd{ std::move(fd) }
	{
	}

	size_t Read(void* buffer, const size_t length)
	{
		return m_fd.Read(buffer, length);
	}

	size_t Send(const void* buffer, const size_t len, const int flags)
	{
		const auto result = send(m_fd.Get(), buffer, len, flags);
		if (result == -1)
		{
			throw std::system_error(errno, std::generic_category());
		}
		return result;
	}

private:
	FileDesc m_fd;
};