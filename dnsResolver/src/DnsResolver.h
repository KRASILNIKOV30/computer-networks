#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <iostream>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

enum class DnsRecordType : uint16_t
{
    A = 1,
    AAAA = 28,
    NS = 2,
    CNAME = 5,
    MX = 15
};

enum class DnsClass : uint16_t
{
    IN = 1
};

struct DnsHeader
{
    uint16_t TransactionId;
    uint16_t Flags;
    uint16_t Questions;
    uint16_t Answers;
    uint16_t Authority;
    uint16_t Additional;
};

struct DnsQuestion
{
    std::string DomainName;
    DnsRecordType Type;
    DnsClass Class;
};

struct DnsResource
{
    std::string Name;
    DnsRecordType Type;
    DnsClass Class;
    uint32_t TTL;
    std::vector<uint8_t> Data;
};

struct DnsResponse
{
    std::vector<DnsResource> Answers;
    std::vector<DnsResource> Authority;
    std::vector<DnsResource> Additional;
    bool Authoritative = false;
};

class DnsResolver
{
public:
    DnsResolver(bool debugMode = false)
        : m_debugMode(debugMode)
    {
    }

    std::vector<std::string> Resolve(const std::string& domain, DnsRecordType recordType)
    {
        if (m_debugMode)
        {
            Log("Starting DNS resolution for domain: " + domain + ", type: " + TypeToString(recordType));
        }

        try
        {
            // Упрощённый алгоритм с использованием системных функций
            auto result = ResolveUsingSystem(domain, recordType);

            if (m_debugMode && result.empty())
            {
                Log("DNS resolution failed for domain: " + domain);
            }
            else if (m_debugMode && !result.empty())
            {
                Log("DNS resolution successful. Found " + std::to_string(result.size()) + " addresses");
            }

            return result;
        }
        catch (const std::exception& e)
        {
            Log("Error during DNS resolution: " + std::string(e.what()));
            return {};
        }
    }

private:
    bool m_debugMode;
    static constexpr uint16_t DNS_PORT = 53;
    static constexpr uint16_t DNS_TIMEOUT = 5000; // milliseconds

    void Log(const std::string& message)
    {
        if (m_debugMode)
        {
            std::cout << "[DNS] " << message << std::endl;
        }
    }

    std::vector<std::string> ResolveUsingSystem(const std::string& domain, DnsRecordType recordType)
    {
        Log("Using system DNS resolution");

        std::vector<std::string> addresses;

        if (recordType == DnsRecordType::A)
        {
            struct addrinfo hints{};
            struct addrinfo* result = nullptr;

            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;

            int status = getaddrinfo(domain.c_str(), nullptr, &hints, &result);
            if (status != 0)
            {
                Log("getaddrinfo failed: " + std::string(gai_strerror(status)));
                return addresses;
            }

            for (struct addrinfo* p = result; p != nullptr; p = p->ai_next)
            {
                if (p->ai_family == AF_INET)
                {
                    struct sockaddr_in* ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
                    char ipstr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, INET_ADDRSTRLEN);
                    addresses.push_back(std::string(ipstr));
                    Log("Found IPv4 address: " + std::string(ipstr));
                }
            }

            freeaddrinfo(result);
        }
        else if (recordType == DnsRecordType::AAAA)
        {
            struct addrinfo hints{};
            struct addrinfo* result = nullptr;

            hints.ai_family = AF_INET6;
            hints.ai_socktype = SOCK_STREAM;

            int status = getaddrinfo(domain.c_str(), nullptr, &hints, &result);
            if (status != 0)
            {
                Log("getaddrinfo failed: " + std::string(gai_strerror(status)));
                return addresses;
            }

            for (struct addrinfo* p = result; p != nullptr; p = p->ai_next)
            {
                if (p->ai_family == AF_INET6)
                {
                    struct sockaddr_in6* ipv6 = reinterpret_cast<struct sockaddr_in6*>(p->ai_addr);
                    char ipstr[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
                    addresses.push_back(std::string(ipstr));
                    Log("Found IPv6 address: " + std::string(ipstr));
                }
            }

            freeaddrinfo(result);
        }
        else
        {
            Log("Record type " + TypeToString(recordType) + " not supported in system mode");
        }

        return addresses;
    }

    static std::string TypeToString(DnsRecordType type)
    {
        switch (type)
        {
            case DnsRecordType::A: return "A";
            case DnsRecordType::AAAA: return "AAAA";
            case DnsRecordType::NS: return "NS";
            case DnsRecordType::CNAME: return "CNAME";
            case DnsRecordType::MX: return "MX";
            default: return "UNKNOWN";
        }
    }

    std::vector<std::string> GetRootServers()
    {

        return {
            "8.8.8.8",        // Google Public DNS
            "1.1.1.1",        // Cloudflare DNS
            "9.9.9.9",        // Quad9
            "208.67.222.222", // OpenDNS
            "8.8.4.4"         // Google DNS Secondary
        };
    }

    std::vector<std::string> ResolveIterative(const std::string& domain, DnsRecordType recordType, const std::vector<std::string>& servers)
    {
        Log("Starting iterative resolution with " + std::to_string(servers.size()) + " servers");

        std::vector<std::string> currentServers = servers;
        const std::string& currentDomain = domain;

        for (size_t attempt = 0; attempt < currentServers.size(); ++attempt)
        {
            const std::string& server = currentServers[attempt];
            Log("Querying server: " + server + " for domain: " + currentDomain);

            auto response = QueryDnsServer(server, currentDomain, recordType);

            if (!response.Answers.empty())
            {
                Log("Found " + std::to_string(response.Answers.size()) + " answers");
                return ExtractAddresses(response.Answers, recordType);
            }

            if (!response.Authority.empty())
            {
                Log("Found " + std::to_string(response.Authority.size()) + " authority records");
                currentServers = ExtractNameServers(response.Authority);
                if (!currentServers.empty())
                {
                    Log("Next level servers: " + std::to_string(currentServers.size()));
                    auto result = ResolveIterative(currentDomain, recordType, currentServers);
                    if (!result.empty())
                    {
                        return result;
                    }
                }
            }

            if (!response.Additional.empty())
            {
                Log("Found " + std::to_string(response.Additional.size()) + " additional records");
                auto additionalServers = ExtractAddresses(response.Additional, DnsRecordType::A);
                if (!additionalServers.empty())
                {
                    currentServers = additionalServers;
                }
            }
        }

        Log("DNS resolution failed after trying all servers");
        return {};
    }

    DnsResponse QueryDnsServer(const std::string& server, const std::string& domain, DnsRecordType recordType)
    {
        Log("Sending DNS query to " + server + " for " + domain);

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0)
        {
            throw std::runtime_error("Failed to create UDP socket");
        }

        struct timeval tv;
        tv.tv_sec = DNS_TIMEOUT / 1000;
        tv.tv_usec = (DNS_TIMEOUT % 1000) * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(DNS_PORT);

        if (inet_pton(AF_INET, server.c_str(), &serverAddr.sin_addr) <= 0)
        {
            close(sock);
            throw std::runtime_error("Invalid server address: " + server);
        }

        auto query = CreateDnsQuery(domain, recordType);

        ssize_t sent = sendto(sock, query.data(), query.size(), 0,
                             reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
        if (sent < 0)
        {
            close(sock);
            throw std::runtime_error("Failed to send DNS query");
        }

        std::vector<uint8_t> responseBuffer(512);
        sockaddr_in responseAddr{};
        socklen_t addrLen = sizeof(responseAddr);
        ssize_t received = recvfrom(sock, responseBuffer.data(), responseBuffer.size(), 0,
                                   reinterpret_cast<sockaddr*>(&responseAddr), &addrLen);

        close(sock);

        if (received < 0)
        {
            throw std::runtime_error("Failed to receive DNS response from " + server);
        }

        responseBuffer.resize(received);
        Log("Received DNS response: " + std::to_string(received) + " bytes");

        return ParseDnsResponse(responseBuffer);
    }

    std::vector<uint8_t> CreateDnsQuery(const std::string& domain, DnsRecordType recordType)
    {
        std::vector<uint8_t> query;

        uint16_t transactionId = 0x1234;
        query.push_back((transactionId >> 8) & 0xFF);
        query.push_back(transactionId & 0xFF);

        // Flags
        query.push_back(0x01);
        query.push_back(0x00);

        // Questions = 1
        query.push_back(0x00);
        query.push_back(0x01);

        // Answers = 0
        query.push_back(0x00);
        query.push_back(0x00);

        // Authority = 0
        query.push_back(0x00);
        query.push_back(0x00);

        // Additional = 0
        query.push_back(0x00);
        query.push_back(0x00);

        std::vector<std::string> labels = SplitDomain(domain);
        for (const auto& label : labels)
        {
            query.push_back(static_cast<uint8_t>(label.length()));
            query.insert(query.end(), label.begin(), label.end());
        }
        query.push_back(0x00);

        // Тип записи
        query.push_back(0x00);
        query.push_back(static_cast<uint16_t>(recordType) >> 8);

        // Класс (IN)
        query.push_back(0x00);
        query.push_back(0x01);

        return query;
    }

    std::vector<std::string> SplitDomain(const std::string& domain)
    {
        std::vector<std::string> labels;
        std::string current;
        
        for (char c : domain)
        {
            if (c == '.')
            {
                if (!current.empty())
                {
                    labels.push_back(current);
                    current.clear();
                }
            }
            else
            {
                current += c;
            }
        }
        
        if (!current.empty())
        {
            labels.push_back(current);
        }
        
        return labels;
    }

    DnsResponse ParseDnsResponse(const std::vector<uint8_t>& data)
    {
        DnsResponse response;

        if (data.size() < 12)
        {
            throw std::runtime_error("Invalid DNS response: too short");
        }

        size_t offset = 0;
        offset += 2; // Transaction ID

        uint16_t flags = (data[offset] << 8) | data[offset + 1];
        response.Authoritative = (flags & 0x0400) != 0; // AA bit
        offset += 2;

        offset += 2; // Questions
        uint16_t answerCount = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        offset += 2; // Authority
        offset += 2; // Additional

        Log("DNS response: " + std::to_string(answerCount) + " answers, authoritative: " + 
            (response.Authoritative ? "yes" : "no"));

        offset = SkipQuestion(data, offset);

        for (uint16_t i = 0; i < answerCount && offset < data.size(); ++i)
        {
            try
            {
                auto resource = ParseResourceRecord(data, offset);
                response.Answers.push_back(resource);
            }
            catch (const std::exception& e)
            {
                Log("Failed to parse answer " + std::to_string(i) + ": " + e.what());
                break;
            }
        }

        return response;
    }

    size_t SkipQuestion(const std::vector<uint8_t>& data, size_t offset)
    {
        while (offset < data.size() && data[offset] != 0)
        {
            offset += data[offset] + 1;
        }
        return offset + 1 + 4; // + null byte + type + class
    }

    DnsResource ParseResourceRecord(const std::vector<uint8_t>& data, size_t& offset)
    {
        DnsResource resource;

        resource.Name = ParseDomainName(data, offset);

        if (offset + 2 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        resource.Type = static_cast<DnsRecordType>((data[offset] << 8) | data[offset + 1]);
        offset += 2;

        if (offset + 2 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        resource.Class = static_cast<DnsClass>((data[offset] << 8) | data[offset + 1]);
        offset += 2;

        if (offset + 4 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        resource.TTL = (data[offset] << 24) | (data[offset + 1] << 16) | 
                      (data[offset + 2] << 8) | data[offset + 3];
        offset += 4;

        if (offset + 2 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        uint16_t dataLength = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        if (offset + dataLength > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        resource.Data.assign(data.begin() + offset, data.begin() + offset + dataLength);
        offset += dataLength;

        return resource;
    }

    std::string ParseDomainName(const std::vector<uint8_t>& data, size_t& offset)
    {
        std::string domain;
        size_t startOffset = offset;

        while (offset < data.size())
        {
            uint8_t length = data[offset];

            if ((length & 0xC0) == 0xC0)
            {
                if (offset + 1 >= data.size())
                    throw std::runtime_error("Invalid compressed domain name");
                uint16_t pointer = ((length & 0x3F) << 8) | data[offset + 1];
                offset += 2;

                size_t savedOffset = offset;
                offset = pointer;
                std::string referenced = ParseDomainName(data, offset);
                offset = savedOffset;

                if (!domain.empty())
                    domain += ".";
                domain += referenced;
                break;
            }
            else if (length == 0)
            {
                offset++;
                break;
            }
            else
            {
                if (offset + length + 1 > data.size())
                    throw std::runtime_error("Invalid domain name label");

                if (!domain.empty())
                    domain += ".";
                domain.append(reinterpret_cast<const char*>(data.data() + offset + 1), length);
                offset += length + 1;
            }
        }

        return domain;
    }

    std::vector<std::string> ExtractAddresses(const std::vector<DnsResource>& resources, DnsRecordType recordType)
    {
        std::vector<std::string> addresses;

        for (const auto& resource : resources)
        {
            if (resource.Type == recordType)
            {
                try
                {
                    std::string address;
                    if (recordType == DnsRecordType::A && resource.Data.size() == 4)
                    {
                        address = std::to_string(resource.Data[0]) + "." +
                                std::to_string(resource.Data[1]) + "." +
                                std::to_string(resource.Data[2]) + "." +
                                std::to_string(resource.Data[3]);
                    }
                    else if (recordType == DnsRecordType::AAAA && resource.Data.size() == 16)
                    {
                        std::stringstream ss;
                        for (int i = 0; i < 16; i += 2)
                        {
                            uint16_t segment = (resource.Data[i] << 8) | resource.Data[i + 1];
                            ss << std::hex << segment;
                            if (i < 14) ss << ":";
                        }
                        address = ss.str();
                    }

                    if (!address.empty())
                    {
                        addresses.push_back(address);
                        Log("Found address: " + address);
                    }
                }
                catch (const std::exception& e)
                {
                    Log("Failed to parse address: " + std::string(e.what()));
                }
            }
        }

        return addresses;
    }

    std::vector<std::string> ExtractNameServers(const std::vector<DnsResource>& resources)
    {
        std::vector<std::string> nameServers;

        for (const auto& resource : resources)
        {
            if (resource.Type == DnsRecordType::NS)
            {
                try
                {
                    std::string nsName = ParseCompressedName(resource.Data);
                    if (!nsName.empty())
                    {
                        nameServers.push_back(nsName);
                        Log("Found nameserver: " + nsName);
                    }
                }
                catch (const std::exception& e)
                {
                    Log("Failed to parse nameserver: " + std::string(e.what()));
                }
            }
        }

        return nameServers;
    }

    std::string ParseCompressedName(const std::vector<uint8_t>& data)
    {
        size_t offset = 0;
        return ParseDomainName(data, offset);
    }
};