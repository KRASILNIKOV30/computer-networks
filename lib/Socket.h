#pragma once
#include "FileDesc.h"
#include <sys/socket.h>
#include <string>

class Socket
{
public:
	explicit Socket(FileDesc fd)
		: m_fd{ std::move(fd) }
	{
        std::cout << "Socket created" << std::endl;
	}

	size_t Read(void* buffer, const size_t length)
	{
        auto bytesRead = m_fd.Read(buffer, length);
        Log("Read message: ", buffer, bytesRead);

		return bytesRead;
	}

	size_t Send(const void* buffer, const size_t len, const int flags)
	{
		const auto result = send(m_fd.Get(), buffer, len, flags);
		if (result == -1)
		{
			throw std::system_error(errno, std::generic_category());
		}
        Log("Send message: ", buffer, len);

		return result;
	}

private:
    static void Log(std::string const& prefix, const void* buffer, const size_t length)
    {
        std::cout << prefix << std::string((char*)buffer, length) << std::endl;
    }

private:
	FileDesc m_fd;
};