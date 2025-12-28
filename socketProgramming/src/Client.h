#pragma once
#include "constants/Constants.h"
#include "../../lib/Connection.h"
#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <cassert>

struct ClientMode
{
	std::string address;
	uint16_t port;
};

namespace
{
    const std::string clientName = "Client of Bogdan";
}

void HandleResponse(std::string const& response, std::string const& clientNumberStr)
{
    const auto delPos = response.find(DELIMITER);
    assert((delPos != std::string::npos));
    const auto serverName = response.substr(0, delPos);
    int serverNumber = std::stoi(response.substr(delPos + 1));
    int clientNumber = std::stoi(clientNumberStr);

    std::cout << "Client Name: " << clientName << std::endl;
    std::cout << "Server Name: " << serverName << std::endl;
    std::cout << "Client Number: " << clientNumber << std::endl;
    std::cout << "Server Number: " << serverNumber << std::endl;
    std::cout << "Sum: " << clientNumber + serverNumber << std::endl;
    std::cout << "-------------------------" << std::endl;
}

inline void Run(const ClientMode& mode)
{
    Connection connection(mode.port, mode.address);
	std::cout << "Connected to server at " << mode.address << ":" << mode.port << std::endl;

	std::string number;
	std::cout << "Enter a number: ";
    std::getline(std::cin, number);

    std::string message = clientName + DELIMITER += number;
    connection.Send(message);
    const auto response = connection.Receive();
    if (response.empty())
    {
        return;
    }

    try
    {
        HandleResponse(response, number);
    }
    catch (std::exception const& e)
    {
        std::cout << e.what() << std::endl;
    }
}