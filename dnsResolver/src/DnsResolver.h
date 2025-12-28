#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <iostream>
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
    NS = 2,
    CNAME = 5,
    MX = 15,
    AAAA = 28
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
    explicit DnsResolver(bool debugMode = false)
            : m_debugMode(debugMode)
    {
        std::random_device rd;
        m_randomEngine.seed(rd());
    }

    std::vector<std::string> Resolve(const std::string& domain, DnsRecordType recordType)
    {
        if (m_debugMode)
            Log("Starting iterative DNS resolution for domain: " + domain + ", type: " + TypeToString(recordType));

        try
        {
            auto result = ResolveIterative(domain, recordType);

            if (m_debugMode)
            {
                if (result.empty())
                    Log("DNS resolution failed for domain: " + domain);
                else
                    Log("DNS resolution successful. Found " + std::to_string(result.size()) + " addresses");
            }

            return result;
        }
        catch (const std::exception& e)
        {
            if (m_debugMode)
                Log("Error during DNS resolution: " + std::string(e.what()));
            return {};
        }
    }

private:
    bool m_debugMode;
    std::mt19937 m_randomEngine;
    static constexpr uint16_t DNS_PORT = 53;
    static constexpr uint16_t DNS_TIMEOUT_MS = 5000;
    static constexpr uint16_t MAX_RECURSION_DEPTH = 10;

    class DnsPacketReader
    {
    public:
        explicit DnsPacketReader(const std::vector<uint8_t>& data)
                : m_data(data), m_offset(0) {}

        uint8_t ReadU8()
        {
            if (m_offset >= m_data.size()) throw std::runtime_error("Unexpected end of packet (U8)");
            return m_data[m_offset++];
        }

        uint16_t ReadU16()
        {
            if (m_offset + 2 > m_data.size()) throw std::runtime_error("Unexpected end of packet (U16)");
            uint16_t value;
            std::memcpy(&value, &m_data[m_offset], 2);
            m_offset += 2;
            return ntohs(value);
        }

        uint32_t ReadU32()
        {
            if (m_offset + 4 > m_data.size()) throw std::runtime_error("Unexpected end of packet (U32)");
            uint32_t value;
            std::memcpy(&value, &m_data[m_offset], 4);
            m_offset += 4;
            return ntohl(value);
        }

        std::vector<uint8_t> ReadBytes(size_t length)
        {
            if (m_offset + length > m_data.size()) throw std::runtime_error("Unexpected end of packet (Bytes)");
            std::vector<uint8_t> bytes(m_data.begin() + m_offset, m_data.begin() + m_offset + length);
            m_offset += length;
            return bytes;
        }

        void Skip(size_t count)
        {
            if (m_offset + count > m_data.size()) throw std::runtime_error("Unexpected end of packet (Skip)");
            m_offset += count;
        }

        std::string ReadDomainName()
        {
            std::string name;
            while (true)
            {
                if (m_offset >= m_data.size()) throw std::runtime_error("Unexpected end of packet (Name)");

                uint8_t len = m_data[m_offset];

                if ((len & 0xC0) == 0xC0)
                {
                    if (m_offset + 1 >= m_data.size()) throw std::runtime_error("Unexpected end of packet (Ptr)");

                    uint16_t pointer = ((len & 0x3F) << 8) | m_data[m_offset + 1];
                    m_offset += 2;

                    std::string suffix = ReadNameAt(pointer, 0);

                    if (!name.empty()) name += ".";
                    name += suffix;
                    return name;
                }
                else if (len == 0)
                {
                    m_offset++;
                    return name;
                }
                else
                {
                    m_offset++;
                    if (m_offset + len > m_data.size()) throw std::runtime_error("Invalid label length");

                    if (!name.empty()) name += ".";
                    name.append(reinterpret_cast<const char*>(&m_data[m_offset]), len);
                    m_offset += len;
                }
            }
        }

        size_t GetOffset() const { return m_offset; }
        void SetOffset(size_t offset)
        {
            if (offset > m_data.size()) throw std::runtime_error("Offset out of bounds");
            m_offset = offset;
        }

    private:
        std::string ReadNameAt(size_t offset, int recursionDepth) const
        {
            if (recursionDepth > 10) throw std::runtime_error("Domain compression loop");

            std::string name;
            size_t current = offset;

            while (true)
            {
                if (current >= m_data.size()) throw std::runtime_error("Unexpected end of packet (At)");

                uint8_t len = m_data[current];

                if ((len & 0xC0) == 0xC0)
                {
                    if (current + 1 >= m_data.size()) throw std::runtime_error("Unexpected end of packet (AtPtr)");
                    uint16_t pointer = ((len & 0x3F) << 8) | m_data[current + 1];

                    std::string suffix = ReadNameAt(pointer, recursionDepth + 1);
                    if (!name.empty()) name += ".";
                    name += suffix;
                    return name;
                }
                else if (len == 0)
                {
                    return name;
                }
                else
                {
                    current++;
                    if (current + len > m_data.size()) throw std::runtime_error("Invalid label length (At)");

                    if (!name.empty()) name += ".";
                    name.append(reinterpret_cast<const char*>(&m_data[current]), len);
                    current += len;
                }
            }
        }

        const std::vector<uint8_t>& m_data;
        size_t m_offset;
    };

    void Log(const std::string& message) const
    {
        std::cout << "[DNS] " << message << std::endl;
    }

    std::vector<std::string> ResolveIterative(const std::string& domain, DnsRecordType recordType)
    {
        if (m_debugMode) Log("Starting iterative resolution from root servers");

        auto rootServers = GetRootServers();
        std::shuffle(rootServers.begin(), rootServers.end(), m_randomEngine);

        return ResolveRecursive(domain, recordType, rootServers, 0);
    }

    std::vector<std::string> ResolveRecursive(const std::string& domain, DnsRecordType recordType,
                                              const std::vector<std::string>& servers, int depth)
    {
        if (depth >= MAX_RECURSION_DEPTH)
        {
            if (m_debugMode) Log("Max recursion depth reached");
            return {};
        }

        for (const auto& server : servers)
        {
            if (m_debugMode) Log("Querying server: " + server + " for domain: " + domain);

            try
            {
                auto response = QueryDnsServer(server, domain, recordType);

                if (!response.Answers.empty())
                {
                    if (m_debugMode) Log("Found " + std::to_string(response.Answers.size()) + " answers");
                    return ExtractAddresses(response.Answers, recordType);
                }

                if (!response.Authority.empty())
                {
                    if (m_debugMode) Log("Found " + std::to_string(response.Authority.size()) + " authority records");

                    auto nextLevelServers = ExtractNameServers(response.Authority);
                    if (!nextLevelServers.empty())
                    {
                        auto nextLevelIPs = ResolveServerIPs(nextLevelServers);
                        if (!nextLevelIPs.empty())
                        {
                            auto result = ResolveRecursive(domain, recordType, nextLevelIPs, depth + 1);
                            if (!result.empty()) return result;
                        }
                    }
                }

                if (!response.Additional.empty())
                {
                    auto additionalIPs = ExtractAddresses(response.Additional, DnsRecordType::A);
                    if (!additionalIPs.empty())
                    {
                        auto result = ResolveRecursive(domain, recordType, additionalIPs, depth + 1);
                        if (!result.empty()) return result;
                    }
                }
            }
            catch (const std::exception& e)
            {
                if (m_debugMode) Log("Failed to query server " + server + ": " + e.what());
            }
        }

        return {};
    }

    std::vector<std::string> ResolveServerIPs(const std::vector<std::string>& serverNames)
    {
        std::vector<std::string> serverIPs;
        serverIPs.reserve(serverNames.size());

        for (const auto& serverName : serverNames)
        {
            try
            {
                auto ips = QueryDnsServerIPs(serverName);
                serverIPs.insert(serverIPs.end(), ips.begin(), ips.end());
            }
            catch (...)
            {
                if (m_debugMode) Log("Skipping resolution for nameserver: " + serverName);
            }
        }
        return serverIPs;
    }

    std::vector<std::string> QueryDnsServerIPs(const std::string& serverName)
    {
        auto rootServers = GetRootServers();
        for (const auto& server : rootServers)
        {
            try
            {
                auto response = QueryDnsServer(server, serverName, DnsRecordType::A);
                auto addresses = ExtractAddresses(response.Answers, DnsRecordType::A);
                if (!addresses.empty()) return addresses;
            }
            catch (const std::exception& e)
            {
                if (m_debugMode) Log("Failed to resolve nameserver IP via " + server + ": " + e.what());
            }
        }
        throw std::runtime_error("Failed to resolve IP for server: " + serverName);
    }

    DnsResponse QueryDnsServer(const std::string& server, const std::string& domain, DnsRecordType recordType)
    {
        FileDesc sockFd(socket(AF_INET, SOCK_DGRAM, 0));
        if (!sockFd.IsOpen()) throw std::runtime_error("Failed to create UDP socket");

        struct timeval tv{DNS_TIMEOUT_MS / 1000, (DNS_TIMEOUT_MS % 1000) * 1000};
        setsockopt(sockFd.Get(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(DNS_PORT);

        if (inet_pton(AF_INET, server.c_str(), &serverAddr.sin_addr) <= 0)
        {
            throw std::runtime_error("Invalid server address: " + server);
        }

        auto query = CreateDnsQuery(domain, recordType);

        if (sendto(sockFd.Get(), query.data(), query.size(), 0,
                   reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
        {
            throw std::runtime_error("Failed to send DNS query");
        }

        std::vector<uint8_t> buffer(512);
        ssize_t received = recvfrom(sockFd.Get(), buffer.data(), buffer.size(), 0, nullptr, nullptr);

        if (received < 0) throw std::runtime_error("Failed to receive DNS response");

        buffer.resize(received);
        if (m_debugMode) Log("Received response: " + std::to_string(received) + " bytes from " + server);

        return ParseDnsResponse(buffer);
    }

    static const std::vector<std::string>& GetRootServers()
    {
        static const std::vector<std::string> servers = {
                "198.41.0.4", "199.9.14.201", "192.33.4.12", "199.7.91.13",
                "192.203.230.10", "192.5.5.241", "192.112.36.4", "128.63.2.53",
                "192.36.148.17", "192.58.128.30", "193.0.14.129", "199.7.83.42", "202.12.27.33"
        };
        return servers;
    }

    std::vector<uint8_t> CreateDnsQuery(const std::string& domain, DnsRecordType recordType)
    {
        std::vector<uint8_t> query;
        query.reserve(512);

        std::uniform_int_distribution<uint16_t> dist(0, 0xFFFF);
        uint16_t id = dist(m_randomEngine);

        auto pushU16 = [&](uint16_t val) {
            uint16_t be = htons(val);
            const uint8_t* p = reinterpret_cast<const uint8_t*>(&be);
            query.push_back(p[0]);
            query.push_back(p[1]);
        };

        pushU16(id);
        pushU16(0x0000);
        pushU16(0x0001);
        pushU16(0x0000);
        pushU16(0x0000);
        pushU16(0x0000);

        std::string label;
        for (char c : domain)
        {
            if (c == '.')
            {
                if (!label.empty())
                {
                    if (label.size() > 63) throw std::runtime_error("Label too long");
                    query.push_back(static_cast<uint8_t>(label.size()));
                    query.insert(query.end(), label.begin(), label.end());
                    label.clear();
                }
            }
            else label += c;
        }

        if (!label.empty())
        {
            query.push_back(static_cast<uint8_t>(label.size()));
            query.insert(query.end(), label.begin(), label.end());
        }
        query.push_back(0);

        pushU16(static_cast<uint16_t>(recordType));
        pushU16(static_cast<uint16_t>(DnsClass::IN));

        return query;
    }

    DnsResponse ParseDnsResponse(const std::vector<uint8_t>& data)
    {
        DnsResponse response;
        DnsPacketReader reader(data);

        reader.Skip(2);
        uint16_t flags = reader.ReadU16();
        response.Authoritative = (flags & 0x0400) != 0;

        uint16_t questionCount = reader.ReadU16();
        uint16_t answerCount = reader.ReadU16();
        uint16_t authorityCount = reader.ReadU16();
        uint16_t additionalCount = reader.ReadU16();

        for (uint16_t i = 0; i < questionCount; ++i)
        {
            reader.ReadDomainName();
            reader.Skip(4);
        }

        auto ParseSection = [&](std::vector<DnsResource>& target, uint16_t count) {
            for (uint16_t i = 0; i < count; ++i)
            {
                DnsResource res;
                res.Name = reader.ReadDomainName();
                res.Type = static_cast<DnsRecordType>(reader.ReadU16());
                res.Class = static_cast<DnsClass>(reader.ReadU16());
                res.TTL = reader.ReadU32();

                uint16_t dataLen = reader.ReadU16();

                size_t nextRecordOffset = reader.GetOffset() + dataLen;

                if (res.Type == DnsRecordType::NS || res.Type == DnsRecordType::CNAME)
                {
                    std::string extractedName = reader.ReadDomainName();
                    res.Data.assign(extractedName.begin(), extractedName.end());
                }
                else
                {
                    res.Data = reader.ReadBytes(dataLen);
                }

                reader.SetOffset(nextRecordOffset);

                target.push_back(std::move(res));
            }
        };

        ParseSection(response.Answers, answerCount);
        ParseSection(response.Authority, authorityCount);
        ParseSection(response.Additional, additionalCount);

        return response;
    }

    std::vector<std::string> ExtractAddresses(const std::vector<DnsResource>& resources, DnsRecordType recordType)
    {
        std::vector<std::string> addresses;
        for (const auto& res : resources)
        {
            if (res.Type == recordType)
            {
                if (recordType == DnsRecordType::A && res.Data.size() == 4)
                {
                    char buf[INET_ADDRSTRLEN];
                    if (inet_ntop(AF_INET, res.Data.data(), buf, sizeof(buf)))
                        addresses.emplace_back(buf);
                }
                else if (recordType == DnsRecordType::AAAA && res.Data.size() == 16)
                {
                    char buf[INET6_ADDRSTRLEN];
                    if (inet_ntop(AF_INET6, res.Data.data(), buf, sizeof(buf)))
                        addresses.emplace_back(buf);
                }
            }
        }
        return addresses;
    }

    std::vector<std::string> ExtractNameServers(const std::vector<DnsResource>& resources)
    {
        std::vector<std::string> nameServers;
        for (const auto& res : resources)
        {
            if (res.Type == DnsRecordType::NS && !res.Data.empty())
            {
                std::string nsName(res.Data.begin(), res.Data.end());
                if (!nsName.empty())
                {
                    nameServers.push_back(nsName);
                }
            }
        }
        return nameServers;
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
            default: return std::to_string(static_cast<uint16_t>(type));
        }
    }
};