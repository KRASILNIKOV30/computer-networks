#pragma once
#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>
#include "../../lib/Socket.h"
#include "../../lib/Acceptor.h"

struct ServerMode
{
	uint16_t port;
};

namespace
{
    const std::string serverName = "Server of Ivan";
    const int serverNumber = 50;
    const int QUEUE_SIZE = 5;
}

inline bool HandleRequest(Socket& clientSocket, std::string const& message)
{
    const auto delPos = message.find(DELIMITER);
    assert((delPos != std::string::npos));
    const auto clientName = message.substr(0, delPos);
    int clientNumber = std::stoi(message.substr(delPos + 1));

    if (clientNumber < 1 || clientNumber > 100)
    {
        return true;
    }

    std::cout << "Client name: " << clientName << std::endl;
    std::cout << "Server name: " << serverName << std::endl;
    std::cout << "Client number: " << clientNumber << std::endl;
    std::cout << "Server number: " << serverNumber << std::endl;
    std::cout << "Sum: " << clientNumber + serverNumber << std::endl;
    std::cout << "-------------------------" << std::endl;

	const std::string response = serverName + DELIMITER += std::to_string(serverNumber);
	clientSocket.Send(response.data(), response.size(), 0);

    return false;
}

inline void HandleClient(Socket&& clientSocket, std::atomic_flag& stopFlag)
{
	char buffer[1024];
	for (size_t bytesRead; (bytesRead = clientSocket.Read(&buffer, sizeof(buffer))) > 0;)
	{
		const auto message = std::string(buffer, bytesRead);
		if (HandleRequest(clientSocket, message))
        {
            stopFlag.test_and_set();
            return;
        }
	}
}

inline void Run(const ServerMode& mode)
{
	const sockaddr_in serverAddr{
		.sin_family = AF_INET,
		.sin_port = htons(mode.port),
		.sin_addr = { .s_addr = INADDR_ANY },
	};

	const Acceptor acceptor{ serverAddr, QUEUE_SIZE };

	std::cout << "Listening to the port " << mode.port << std::endl;

    std::atomic_flag stopFlag = ATOMIC_FLAG_INIT;
	while (!stopFlag.test())
	{
		std::cout << "Accepting" << std::endl;
		auto clientSocket = acceptor.Accept();
		std::cout << "Accepted" << std::endl;
		HandleClient(std::move(clientSocket), stopFlag);
	}
}