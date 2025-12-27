#include "../lib/UdpSocket.h"
#include <iostream>
#include <string>
#include <random>
#include <ctime>

int main(int argc, char* argv[]) {
    int port = 12345;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distrib(1, 10);

    try {
        UdpSocket serverSocket;
        serverSocket.Bind(port);

        std::cout << "Server acts as a standard Ping Echo server." << std::endl;
        std::cout << "Simulating 30% packet loss..." << std::endl;

        while (true) {
            sockaddr_in clientAddr{};
            try {
                std::string message = serverSocket.RecvFrom(clientAddr);

                char clientIp[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);
                int clientPort = ntohs(clientAddr.sin_port);

                std::cout << "Received from " << clientIp << ":" << clientPort
                          << " -> " << message << std::endl;

                if (distrib(gen) <= 3) {
                    std::cout << "   [Packet Lost Simulated]" << std::endl;
                    continue;
                }

                serverSocket.SendTo(message, clientAddr);
                std::cout << "   [Reply Sent]" << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "Error during communication: " << e.what() << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}