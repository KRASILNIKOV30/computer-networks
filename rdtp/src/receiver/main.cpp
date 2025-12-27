#include <iostream>
#include "GbnReceiver.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <outfile> [-d]" << std::endl;
        return 1;
    }

    uint16_t port = std::stoi(argv[1]);
    std::string file = argv[2];
    bool debug = (argc > 3 && std::string(argv[3]) == "-d");

    try {
        GbnReceiver receiver(port, file, debug);
        receiver.Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
