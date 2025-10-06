#include "src/lib/Acceptor.h"
#include "src/Client.h"
#include "src/Server.h"
#include <filesystem>
#include <iostream>
#include <variant>

namespace fs = std::filesystem;

struct HelpMode
{
};

using ProgramMode = std::variant<HelpMode, ClientMode, ServerMode>;

ProgramMode ParseCommandLine(const int argc, char* argv[])
{
	if (argc < 2)
	{
		throw std::runtime_error(
			"Invalid command line. Type " + fs::path(argv[0]).filename().string() + " -h for help.");
	}

	if (argc == 2 && std::string(argv[1]) == "-h")
	{
		return HelpMode{};
	}

	if (std::string(argv[1]) == "server")
	{
		if (argc != 3)
		{
			throw std::runtime_error("Invalid server command line parameters");
		}
		unsigned long port = std::stoul(argv[2]);
		if (port < 1 || port >= std::numeric_limits<uint16_t>::max())
		{
			throw std::runtime_error("Invalid port");
		}
		return ServerMode{ .port = static_cast<uint16_t>(port) };
	}

	if (std::string(argv[1]) == "client")
	{
		if (argc != 4)
		{
			throw std::runtime_error("Invalid client command line parameters");
		}
		const unsigned long port = std::stoul(argv[3]);
		if (port < 1 || port >= std::numeric_limits<uint16_t>::max())
		{
			throw std::runtime_error("Invalid port");
		}
		return ClientMode{ .address = argv[2], .port = static_cast<uint16_t>(port) };
	}
	throw std::runtime_error("Invalid server command line parameters");
}

void Run(HelpMode)
{
	std::cout << "socket-programming server PORT" << std::endl;
	std::cout << "socket-programming client ADDRESS PORT" << std::endl;
}

int main(int argc, char* argv[])
{
	try
	{
		auto mode = ParseCommandLine(argc, argv);
		std::visit([](const auto& m) { Run(m); }, mode);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
