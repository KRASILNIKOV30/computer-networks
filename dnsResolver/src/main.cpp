#include "DnsResolver.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

struct DnsMode
{
    std::string domain;
    std::string recordType;
    bool debugMode = false;
};

DnsMode ParseCommandLine(int argc, char* argv[])
{
    if (argc < 3)
    {
        throw std::runtime_error(
            "Usage: " + std::string(argv[0]) + " <domain> <record_type> [-d]"
        );
    }

    DnsMode mode;
    mode.domain = argv[1];
    mode.recordType = argv[2];

    // Преобразуем в верхний регистр для сравнения
    std::string upperType = mode.recordType;
    std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);

    if (upperType == "A" || upperType == "AAAA" || upperType == "NS" || upperType == "CNAME" || upperType == "MX")
    {
        // Корректный тип записи
    }
    else
    {
        throw std::runtime_error("Invalid record type: " + mode.recordType + 
                               ". Supported types: A, AAAA, NS, CNAME, MX");
    }

    // Проверяем флаг отладки
    for (int i = 3; i < argc; ++i)
    {
        if (std::string(argv[i]) == "-d")
        {
            mode.debugMode = true;
            break;
        }
    }

    return mode;
}

DnsRecordType StringToRecordType(const std::string& typeStr)
{
    std::string upperType = typeStr;
    std::transform(upperType.begin(), upperType.end(), upperType.begin(), ::toupper);

    if (upperType == "A") return DnsRecordType::A;
    if (upperType == "AAAA") return DnsRecordType::AAAA;
    if (upperType == "NS") return DnsRecordType::NS;
    if (upperType == "CNAME") return DnsRecordType::CNAME;
    if (upperType == "MX") return DnsRecordType::MX;

    throw std::runtime_error("Unknown record type: " + typeStr);
}

void Run(const DnsMode& mode)
{
    std::cout << "DNS Resolver starting..." << std::endl;
    std::cout << "Domain: " << mode.domain << std::endl;
    std::cout << "Record type: " << mode.recordType << std::endl;
    std::cout << "Debug mode: " << (mode.debugMode ? "enabled" : "disabled") << std::endl;

    try
    {
        DnsResolver resolver(mode.debugMode);
        auto recordType = StringToRecordType(mode.recordType);
        auto addresses = resolver.Resolve(mode.domain, recordType);

        if (!addresses.empty())
        {
            std::cout << "\n=== RESULT ===" << std::endl;
            std::cout << "Found " << addresses.size() << " address(es):" << std::endl;
            for (const auto& address : addresses)
            {
                std::cout << address << std::endl;
            }
        }
        else
        {
            std::cout << "\n=== RESULT ===" << std::endl;
            std::cout << "No addresses found for domain: " << mode.domain << std::endl;
            std::cout << "Possible reasons:" << std::endl;
            std::cout << "- Domain does not exist" << std::endl;
            std::cout << "- No records of type '" << mode.recordType << "' found" << std::endl;
            std::cout << "- Network connectivity issues" << std::endl;
            std::cout << "- DNS server timeout" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cout << "\n=== ERROR ===" << std::endl;
        std::cout << "DNS resolution failed: " << e.what() << std::endl;
        std::cout << "Please check your internet connection and domain name." << std::endl;
        throw;
    }
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
        std::cerr << "\nUsage examples:" << std::endl;
        std::cerr << "  " << argv[0] << " example.com A" << std::endl;
        std::cerr << "  " << argv[0] << " example.com AAAA -d" << std::endl;
        std::cerr << "  " << argv[0] << " google.com A" << std::endl;
        std::cerr << "\nSupported record types: A, AAAA, NS, CNAME, MX" << std::endl;
        return EXIT_FAILURE;
    }
}
