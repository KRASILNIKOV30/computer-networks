#include "../lib/UdpSocket.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <iomanip>

struct Config {
    std::string ip = "127.0.0.1";
    int port = 12345;
};

sockaddr_in CreateSockAddr(const std::string& ip, int port) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        throw std::runtime_error("Invalid address/ Address not supported");
    }
    return addr;
}

int main(int argc, char* argv[]) {
    try {
        Config config;
        if (argc > 1) config.ip = argv[1];
        if (argc > 2) config.port = std::stoi(argv[2]);

        sockaddr_in serverAddr = CreateSockAddr(config.ip, config.port);

        UdpSocket socket;
        socket.SetRecvTimeout(1);

        std::cout << "Pinging " << config.ip << ":" << config.port << " with 100 packets:" << std::endl;

        for (int seq = 1; seq <= 100; ++seq) {
            auto now = std::chrono::system_clock::now();
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count();

            std::string message = "Ping " + std::to_string(seq) + " " + std::to_string(timestamp);

            auto sendTime = std::chrono::high_resolution_clock::now();

            try {
                socket.SendTo(message, serverAddr);

                sockaddr_in senderAddr{};
                std::string reply = socket.RecvFrom(senderAddr);

                auto recvTime = std::chrono::high_resolution_clock::now();

                std::chrono::duration<double> rtt = recvTime - sendTime;

                std::cout << "Ответ от сервера: " << reply
                          << ", RTT = " << std::fixed << std::setprecision(3) << rtt.count() << " сек"
                          << std::endl;
            } catch (const std::runtime_error& e) {
                if (std::string(e.what()) == "Request timed out") {
                    std::cout << "Request timed out" << std::endl;
                } else {
                    throw;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}