#include <iostream>
#include "GbnSender.h"

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <file> [-d]" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    uint16_t port = std::stoi(argv[2]);
    std::string file = argv[3];
    bool debug = (argc > 4 && std::string(argv[4]) == "-d");

    try {
        GbnSender sender(host, port, file, debug);
        sender.Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}