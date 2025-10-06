#pragma once
#include "lib/Connection.h"
#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <string>

struct ClientMode
{
	std::string address;
	uint16_t port;
};

inline void Run(const ClientMode& mode)
{
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(mode.port);

	Connection connection(serverAddr, mode.address);
	std::cout << "Connected to server at " << mode.address << ":" << mode.port << std::endl;

	std::string command;
	while (std::getline(std::cin, command))
	{
		connection.Send(command);
		const auto response = connection.Receive();
		std::cout << response << std::endl;
	}
}