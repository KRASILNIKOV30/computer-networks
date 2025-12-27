#pragma once
#include <cstdint>
#include <vector>
#include <cstring>
#include <numeric>
#include <arpa/inet.h>

constexpr uint16_t MAGIC_NUMBER = 0xC0DE;
constexpr size_t MAX_PAYLOAD_SIZE = 1400;
constexpr size_t HEADER_SIZE = 12;

enum class PacketType : uint8_t {
    SYN = 0x01,
    ACK = 0x02,
    FIN = 0x04,
    DATA = 0x08
};

struct Header {
    uint32_t seqNum;
    uint8_t flags;
    uint8_t reserved{0};
    uint16_t dataLen;
    uint16_t checksum;
    uint16_t magic;
};

struct Packet {
    Header header{};
    std::vector<uint8_t> payload;

    static uint16_t CalculateChecksum(const std::vector<uint8_t>& data) {
        uint32_t sum = 0;
        for (size_t i = 0; i < data.size(); ++i) {
            sum += data[i];
            if (sum & 0xFFFF0000) {
                sum &= 0xFFFF;
                sum++;
            }
        }
        return static_cast<uint16_t>(~(sum & 0xFFFF));
    }

    std::vector<uint8_t> Serialize() {
        header.dataLen = static_cast<uint16_t>(payload.size());
        header.magic = MAGIC_NUMBER;
        header.checksum = 0;

        std::vector<uint8_t> buffer(HEADER_SIZE + payload.size());

        Header netHeader = header;
        netHeader.seqNum = htonl(header.seqNum);
        netHeader.dataLen = htons(header.dataLen);
        netHeader.magic = htons(header.magic);

        std::memcpy(buffer.data(), &netHeader, HEADER_SIZE);
        std::memcpy(buffer.data() + HEADER_SIZE, payload.data(), payload.size());

        header.checksum = CalculateChecksum(buffer);
        netHeader.checksum = htons(header.checksum);
        std::memcpy(buffer.data() + 8, &netHeader.checksum, 2); // offset 8 is checksum

        return buffer;
    }

    static bool Deserialize(const std::vector<uint8_t>& buffer, Packet& outPacket) {
        if (buffer.size() < HEADER_SIZE) return false;

        Header netHeader;
        std::memcpy(&netHeader, buffer.data(), HEADER_SIZE);

        if (ntohs(netHeader.magic) != MAGIC_NUMBER) return false;

        uint16_t receivedChecksum = ntohs(netHeader.checksum);
        std::vector<uint8_t> checkBuf = buffer;
        std::memset(checkBuf.data() + 8, 0, 2);

        if (CalculateChecksum(checkBuf) != receivedChecksum) return false;

        outPacket.header.seqNum = ntohl(netHeader.seqNum);
        outPacket.header.flags = netHeader.flags;
        outPacket.header.dataLen = ntohs(netHeader.dataLen);
        outPacket.header.checksum = receivedChecksum;
        outPacket.header.magic = ntohs(netHeader.magic);

        if (buffer.size() < HEADER_SIZE + outPacket.header.dataLen) return false;

        outPacket.payload.assign(
                buffer.begin() + HEADER_SIZE,
                buffer.begin() + HEADER_SIZE + outPacket.header.dataLen
        );

        return true;
    }
};