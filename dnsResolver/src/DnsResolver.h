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
#include <algorithm>
#include <stdexcept>
#include <random>
#include "../../lib/FileDesc.h"

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
        std::random_device rd;
        m_randomEngine.seed(rd());
    }

    std::vector<std::string> Resolve(const std::string& domain, DnsRecordType recordType)
    {
        if (m_debugMode)
        {
            Log("Starting iterative DNS resolution for domain: " + domain + ", type: " + TypeToString(recordType));
        }

        try
        {
            // Итеративное разрешение начиная с корневых серверов
            auto result = ResolveIterative(domain, recordType);

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
    std::mt19937 m_randomEngine;
    static constexpr uint16_t DNS_PORT = 53;
    static constexpr uint16_t DNS_TIMEOUT = 5000;
    static constexpr uint16_t MAX_RECURSION_DEPTH = 10;

    void Log(const std::string& message)
    {
        if (m_debugMode)
        {
            std::cout << "[DNS] " << message << std::endl;
        }
    }

    std::vector<std::string> ResolveIterative(const std::string& domain, DnsRecordType recordType)
    {
        Log("Starting iterative resolution from root servers");

        // Получаем список корневых серверов
        auto rootServers = GetRootServers();
        std::shuffle(rootServers.begin(), rootServers.end(), m_randomEngine);

        std::vector<std::string> currentServers = rootServers;
        std::string currentDomain = domain;

        return ResolveRecursive(currentDomain, recordType, currentServers, 0);
    }

    std::vector<std::string> ResolveRecursive(const std::string& domain, DnsRecordType recordType, 
                                             const std::vector<std::string>& servers, int depth)
    {
        if (depth >= MAX_RECURSION_DEPTH)
        {
            Log("Max recursion depth reached");
            return {};
        }

        for (const auto& server : servers)
        {
            Log("Querying server: " + server + " for domain: " + domain);

            try
            {
                // Запрашиваем у текущего сервера
                auto response = QueryDnsServer(server, domain, recordType);

                // Проверяем ответы
                if (!response.Answers.empty())
                {
                    Log("Found " + std::to_string(response.Answers.size()) + " answers");
                    return ExtractAddresses(response.Answers, recordType);
                }

                // Если есть авторитетные записи, переходим к следующему уровню
                if (!response.Authority.empty())
                {
                    Log("Found " + std::to_string(response.Authority.size()) + " authority records");
                    
                    // Извлекаем NS записи для следующего уровня
                    auto nextLevelServers = ExtractNameServers(response.Authority);
                    if (!nextLevelServers.empty())
                    {
                        Log("Next level servers: " + std::to_string(nextLevelServers.size()));
                        
                        // Получаем IP адреса для этих серверов
                        auto nextLevelIPs = ResolveServerIPs(nextLevelServers);
                        if (!nextLevelIPs.empty())
                        {
                            Log("Found " + std::to_string(nextLevelIPs.size()) + " server IPs");
                            
                            // Рекурсивно продолжаем для следующего уровня
                            auto result = ResolveRecursive(domain, recordType, nextLevelIPs, depth + 1);
                            if (!result.empty())
                            {
                                return result;
                            }
                        }
                    }
                }

                // Проверяем дополнительные записи
                if (!response.Additional.empty())
                {
                    Log("Found " + std::to_string(response.Additional.size()) + " additional records");
                    auto additionalIPs = ExtractAddresses(response.Additional, DnsRecordType::A);
                    if (!additionalIPs.empty())
                    {
                        // Пробуем с дополнительными IP
                        auto result = ResolveRecursive(domain, recordType, additionalIPs, depth + 1);
                        if (!result.empty())
                        {
                            return result;
                        }
                    }
                }
            }
            catch (const std::exception& e)
            {
                Log("Failed to query server " + server + ": " + e.what());
                continue;
            }
        }

        Log("DNS resolution failed for domain: " + domain);
        return {};
    }

    std::vector<std::string> ResolveServerIPs(const std::vector<std::string>& serverNames)
    {
        Log("Resolving IPs for " + std::to_string(serverNames.size()) + " servers");

        std::vector<std::string> serverIPs;

        for (const auto& serverName : serverNames)
        {
            try
            {
                Log("Resolving IP for server: " + serverName);
                auto serverIPs_list = QueryDnsServerIPs(serverName);
                serverIPs.insert(serverIPs.end(), serverIPs_list.begin(), serverIPs_list.end());
            }
            catch (const std::exception& e)
            {
                Log("Failed to resolve IP for server " + serverName + ": " + e.what());
            }
        }

        return serverIPs;
    }

    std::vector<std::string> QueryDnsServerIPs(const std::string& serverName)
    {
        Log("Querying IP for server: " + serverName);

        auto rootServers = GetRootServers();
        for (const auto& server : rootServers)
        {
            try
            {
                auto response = QueryDnsServer(server, serverName, DnsRecordType::A);
                auto addresses = ExtractAddresses(response.Answers, DnsRecordType::A);
                if (!addresses.empty())
                {
                    Log("Found IP for " + serverName + ": " + addresses[0]);
                    return addresses;
                }
            }
            catch (const std::exception& e)
            {
                Log("Failed to query " + server + " for " + serverName + ": " + e.what());
                continue;
            }
        }

        throw std::runtime_error("Failed to resolve IP for server: " + serverName);
    }

    DnsResponse QueryDnsServer(const std::string& server, const std::string& domain, DnsRecordType recordType)
    {
        Log("Sending DNS query to " + server + " for " + domain);

        // Создаем UDP сокет используя FileDesc
        FileDesc sockFd(socket(AF_INET, SOCK_DGRAM, 0));
        if (!sockFd.IsOpen())
        {
            throw std::runtime_error("Failed to create UDP socket");
        }

        // Устанавливаем таймаут
        struct timeval tv;
        tv.tv_sec = DNS_TIMEOUT / 1000;
        tv.tv_usec = (DNS_TIMEOUT % 1000) * 1000;
        
        if (setsockopt(sockFd.Get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            Log("Warning: Failed to set socket timeout");
        }

        // Настраиваем адрес DNS сервера
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(DNS_PORT);

        if (inet_pton(AF_INET, server.c_str(), &serverAddr.sin_addr) <= 0)
        {
            throw std::runtime_error("Invalid server address: " + server);
        }

        // Создаем DNS запрос
        auto query = CreateDnsQuery(domain, recordType);
        
        // Отправляем запрос используя FileDesc
        ssize_t sent = sendto(sockFd.Get(), query.data(), query.size(), 0,
                             reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr));
        if (sent < 0)
        {
            throw std::runtime_error("Failed to send DNS query");
        }
        
        Log("Sent " + std::to_string(sent) + " bytes to " + server);

        // Получаем ответ
        std::vector<uint8_t> responseBuffer(512);
        sockaddr_in responseAddr{};
        socklen_t addrLen = sizeof(responseAddr);
        ssize_t received = recvfrom(sockFd.Get(), responseBuffer.data(), responseBuffer.size(), 0,
                                   reinterpret_cast<sockaddr*>(&responseAddr), &addrLen);

        if (received < 0)
        {
            throw std::runtime_error("Failed to receive DNS response from " + server);
        }

        responseBuffer.resize(received);
        Log("Received DNS response: " + std::to_string(received) + " bytes");

        return ParseDnsResponse(responseBuffer);
    }

    std::vector<std::string> GetRootServers()
    {
        // Список корневых DNS серверов
        return {
            "198.41.0.4",     // a.root-servers.net
            "199.9.14.201",   // b.root-servers.net
            "192.33.4.12",    // c.root-servers.net
            "199.7.91.13",    // d.root-servers.net
            "192.203.230.10", // e.root-servers.net
            "192.5.5.241",    // f.root-servers.net
            "192.112.36.4",   // g.root-servers.net
            "128.63.2.53",    // h.root-servers.net
            "192.36.148.17",  // i.root-servers.net
            "192.58.128.30",  // j.root-servers.net
            "193.0.14.129",   // k.root-servers.net
            "199.7.83.42",    // l.root-servers.net
            "202.12.27.33"    // m.root-servers.net
        };
    }

    std::vector<uint8_t> CreateDnsQuery(const std::string& domain, DnsRecordType recordType)
    {
        std::vector<uint8_t> query;

        // Transaction ID (случайный)
        std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);
        uint16_t transactionId = dist(m_randomEngine);
        query.push_back((transactionId >> 8) & 0xFF);
        query.push_back(transactionId & 0xFF);

        // Flags (стандартный запрос)
        query.push_back(0x01); // recursion desired
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

        // Доменное имя в формате DNS
        std::vector<std::string> labels = SplitDomain(domain);
        for (const auto& label : labels)
        {
            if (label.length() > 63)
            {
                throw std::runtime_error("DNS label too long: " + label);
            }
            query.push_back(static_cast<uint8_t>(label.length()));
            query.insert(query.end(), label.begin(), label.end());
        }
        query.push_back(0x00); // конец доменного имени

        // Тип записи
        uint16_t type = static_cast<uint16_t>(recordType);
        query.push_back((type >> 8) & 0xFF);
        query.push_back(type & 0xFF);

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

        // Парсим заголовок
        size_t offset = 0;
        offset += 2; // Transaction ID

        uint16_t flags = (data[offset] << 8) | data[offset + 1];
        response.Authoritative = (flags & 0x0400) != 0; // AA bit
        offset += 2;

        uint16_t questionCount = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        uint16_t answerCount = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        uint16_t authorityCount = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        uint16_t additionalCount = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        Log("DNS response: " + std::to_string(answerCount) + " answers, " + 
            std::to_string(authorityCount) + " authority, " +
            std::to_string(additionalCount) + " additional");

        // Пропускаем вопрос
        offset = SkipQuestion(data, offset);

        // Парсим ответы
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

        // Парсим авторитетные записи
        for (uint16_t i = 0; i < authorityCount && offset < data.size(); ++i)
        {
            try
            {
                auto resource = ParseResourceRecord(data, offset);
                response.Authority.push_back(resource);
            }
            catch (const std::exception& e)
            {
                Log("Failed to parse authority " + std::to_string(i) + ": " + e.what());
                break;
            }
        }

        // Парсим дополнительные записи
        for (uint16_t i = 0; i < additionalCount && offset < data.size(); ++i)
        {
            try
            {
                auto resource = ParseResourceRecord(data, offset);
                response.Additional.push_back(resource);
            }
            catch (const std::exception& e)
            {
                Log("Failed to parse additional " + std::to_string(i) + ": " + e.what());
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

        // Имя (может быть сжатым)
        resource.Name = ParseDomainName(data, offset);

        // Тип
        if (offset + 2 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        resource.Type = static_cast<DnsRecordType>((data[offset] << 8) | data[offset + 1]);
        offset += 2;

        // Класс
        if (offset + 2 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        resource.Class = static_cast<DnsClass>((data[offset] << 8) | data[offset + 1]);
        offset += 2;

        // TTL
        if (offset + 4 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        resource.TTL = (data[offset] << 24) | (data[offset + 1] << 16) | 
                      (data[offset + 2] << 8) | data[offset + 3];
        offset += 4;

        // Длина данных
        if (offset + 2 > data.size())
            throw std::runtime_error("Invalid resource record: truncated");
        uint16_t dataLength = (data[offset] << 8) | data[offset + 1];
        offset += 2;

        // Данные
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
                // Сжатая ссылка
                if (offset + 1 >= data.size())
                    throw std::runtime_error("Invalid compressed domain name");
                uint16_t pointer = ((length & 0x3F) << 8) | data[offset + 1];
                offset += 2;

                // Рекурсивно парсим ссылку
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
                // Конец доменного имени
                offset++;
                break;
            }
            else
            {
                // Обычная метка
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
                        // IPv4 адрес
                        address = std::to_string(resource.Data[0]) + "." +
                                std::to_string(resource.Data[1]) + "." +
                                std::to_string(resource.Data[2]) + "." +
                                std::to_string(resource.Data[3]);
                    }
                    else if (recordType == DnsRecordType::AAAA && resource.Data.size() == 16)
                    {
                        // IPv6 адрес
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
                    std::string nsName = ParseDomainNameFromData(resource.Data);
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

    std::string ParseDomainNameFromData(const std::vector<uint8_t>& data)
    {
        size_t offset = 0;
        return ParseDomainName(data, offset);
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
};