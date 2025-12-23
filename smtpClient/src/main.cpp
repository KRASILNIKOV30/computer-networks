#include "SmtpClient.h"
#include <iostream>
#include <stdexcept>

struct SmtpMode
{
    std::string serverAddress;
    uint16_t port;
    std::string from;
    std::string to;
    std::string subject;
    std::string body;
};

SmtpMode ParseCommandLine(int argc, char* argv[])
{
    if (argc != 7)
    {
        throw std::runtime_error(
            "Usage: " + std::string(argv[0]) + 
            " <server> <port> <from> <to> <subject> <body>"
        );
    }

    SmtpMode mode;
    mode.serverAddress = argv[1];
    mode.port = static_cast<uint16_t>(std::stoul(argv[2]));
    mode.from = argv[3];
    mode.to = argv[4];
    mode.subject = argv[5];

    for (int i = 6; i < argc; ++i)
    {
        if (i > 6) mode.body += " ";
        mode.body += argv[i];
    }
    
    return mode;
}

void Run(const SmtpMode& mode)
{
    std::cout << "Connecting to SMTP server " << mode.serverAddress << ":" << mode.port << "..." << std::endl;
    
    SmtpClient client(mode.serverAddress, mode.port);
    
    std::cout << "Sending email from " << mode.from << " to " << mode.to << "..." << std::endl;
    
    client.SendEmail(mode.from, mode.to, mode.subject, mode.body);
    
    std::cout << "Email sent successfully!" << std::endl;
}

int main(int argc, char* argv[])
{
    try
    {
        auto mode = ParseCommandLine(argc, argv);
        Run(mode);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}