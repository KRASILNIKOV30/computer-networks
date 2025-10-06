#pragma once
#include <cstdint>
#include <iostream>
#include <numeric>
#include <sstream>
#include <thread>

struct ServerMode
{
	uint16_t port;
};

inline int Calculate(const char sign, std::vector<int> const& numbers)
{
	int result;

	if (sign == '+')
	{
		result = std::reduce(numbers.begin(), numbers.end(), 0, std::plus());
	}
	else if (sign == '-')
	{
		result = std::reduce(numbers.begin() + 1, numbers.end(), numbers[0], std::minus());
	}
	else
	{
		throw std::runtime_error("Unknown sign");
	}

	return result;
}

inline int CalculateCommand(const std::string& command)
{
	auto stream = std::istringstream(command);
	char sign;
	stream >> sign;
	std::vector<int> numbers;
	int number;
	while (stream >> number)
	{
		numbers.push_back(number);
	}

	if (numbers.empty())
	{
		throw std::runtime_error("no numbers in command");
	}

	if (!stream.eof())
	{
		throw std::runtime_error("invalid command");
	}

	return Calculate(sign, numbers);
}

inline void HandleRequest(Socket& clientSocket, std::string const& command)
{
	int result;
	try
	{
		result = CalculateCommand(command);
	}
	catch (const std::exception& e)
	{
		const std::string errorMessage = e.what();
		clientSocket.Send(errorMessage.data(), errorMessage.size(), 0);
		return;
	}
	const auto resultStr = std::to_string(result);
	clientSocket.Send(resultStr.data(), resultStr.size(), 0);
}

inline void HandleClient(Socket&& clientSocket)
{
	char buffer[1024];
	for (size_t bytesRead; (bytesRead = clientSocket.Read(&buffer, sizeof(buffer))) > 0;)
	{
		const auto command = std::string(buffer, bytesRead);
		HandleRequest(clientSocket, command);
	}
}

[[noreturn]] inline void Run(const ServerMode& mode)
{
	const sockaddr_in serverAddr{
		.sin_family = AF_INET,
		.sin_port = htons(mode.port),
		.sin_addr = { .s_addr = INADDR_ANY },
	};

	const Acceptor acceptor{ serverAddr, 5 };

	std::cout << "Listening to the port " << mode.port << std::endl;

	std::vector<std::jthread> threads;
    threads.reserve(1024);
	while (true)
	{
		std::cout << "Accepting" << std::endl;
		auto clientSocket = acceptor.Accept();
		std::cout << "Accepted" << std::endl;
		threads.emplace_back(HandleClient, std::move(clientSocket));
	}
}