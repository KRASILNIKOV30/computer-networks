#pragma once
#include "../common/RdtSocket.h"
#include <fstream>
#include <chrono>
#include <deque>

class GbnSender {
public:
    GbnSender(const std::string& host, uint16_t port, const std::string& filename, bool debug)
            : m_targetHost(host), m_targetPort(port), m_filename(filename), m_debug(debug)
    {
        if (inet_pton(AF_INET, host.c_str(), &m_targetAddr.sin_addr) <= 0) {
            throw std::runtime_error("Invalid IP address");
        }
        m_targetAddr.sin_family = AF_INET;
        m_targetAddr.sin_port = htons(port);
    }

    void Run() {
        LoadFile();
        Handshake();
        TransferLoop();
        Teardown();
    }

private:
    std::string m_targetHost;
    uint16_t m_targetPort;
    sockaddr_in m_targetAddr{};
    std::string m_filename;
    bool m_debug;

    RdtSocket m_socket;
    std::vector<uint8_t> m_fileData;

    uint32_t m_base = 0;
    uint32_t m_nextSeqNum = 0;
    uint32_t m_windowSize = 10;
    int m_timeoutMs = 100;

    void Log(const std::string& msg) {
        if (m_debug) std::cout << "[SENDER] " << msg << std::endl;
    }

    void LoadFile() {
        std::ifstream file(m_filename, std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open file: " + m_filename);
        m_fileData = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        Log("File loaded. Size: " + std::to_string(m_fileData.size()) + " bytes");
    }

    void Handshake() {
        Packet syn;
        syn.header.flags = static_cast<uint8_t>(PacketType::SYN);
        syn.header.seqNum = 0;

        Log("Sending SYN...");
        while (true) {
            m_socket.SendTo(syn, m_targetAddr);
            m_socket.SetTimeout(m_timeoutMs);

            Packet ack;
            if (m_socket.RecvFrom(ack)) {
                if (ack.header.flags & static_cast<uint8_t>(PacketType::ACK) &&
                    ack.header.flags & static_cast<uint8_t>(PacketType::SYN)) {
                    Log("Received SYN-ACK");
                    m_base = 1;
                    m_nextSeqNum = 1;
                    return;
                }
            }
            Log("Timeout SYN. Retrying...");
        }
    }

    Packet CreateDataPacket(uint32_t seq) {
        Packet p;
        p.header.seqNum = seq;
        p.header.flags = static_cast<uint8_t>(PacketType::DATA);

        size_t offset = (seq - 1) * MAX_PAYLOAD_SIZE;
        size_t remaining = m_fileData.size() - offset;
        size_t size = std::min(remaining, MAX_PAYLOAD_SIZE);

        p.payload.assign(m_fileData.begin() + offset, m_fileData.begin() + offset + size);
        return p;
    }

    void TransferLoop() {
        uint32_t totalPackets = (m_fileData.size() + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;
        if (m_fileData.empty()) totalPackets = 0;

        auto startTime = std::chrono::high_resolution_clock::now();

        while (m_base <= totalPackets) {
            while (m_nextSeqNum < m_base + m_windowSize && m_nextSeqNum <= totalPackets) {
                Packet p = CreateDataPacket(m_nextSeqNum);
                m_socket.SendTo(p, m_targetAddr);
                Log("Sent Packet #" + std::to_string(m_nextSeqNum));
                m_nextSeqNum++;
            }

            m_socket.SetTimeout(m_timeoutMs);
            Packet ack;
            if (m_socket.RecvFrom(ack)) {
                if (ack.header.flags & static_cast<uint8_t>(PacketType::ACK)) {
                    uint32_t ackNum = ack.header.seqNum;
                    Log("Received ACK #" + std::to_string(ackNum));

                    if (ackNum >= m_base) {
                        m_base = ackNum + 1;

                        if (!m_debug) {
                            float progress = (float)(m_base-1) / totalPackets * 100.0f;
                            std::cout << "\rProgress: " << (int)progress << "%" << std::flush;
                        }
                    }
                }
            } else {
                Log("Timeout! Resending window from " + std::to_string(m_base));
                m_nextSeqNum = m_base;
            }
        }
        if (!m_debug) std::cout << std::endl;

        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cout << "Transfer complete in " << duration << " ms." << std::endl;
    }

    void Teardown() {
        Packet fin;
        fin.header.flags = static_cast<uint8_t>(PacketType::FIN);
        fin.header.seqNum = m_nextSeqNum;

        Log("Sending FIN...");
        int retries = 0;
        while (retries < 5) {
            m_socket.SendTo(fin, m_targetAddr);
            m_socket.SetTimeout(m_timeoutMs);
            Packet ack;
            if (m_socket.RecvFrom(ack)) {
                if (ack.header.flags & static_cast<uint8_t>(PacketType::ACK) &&
                    ack.header.flags & static_cast<uint8_t>(PacketType::FIN)) {
                    Log("Received FIN-ACK. Goodbye.");
                    return;
                }
            }
            retries++;
            Log("Timeout FIN. Retry " + std::to_string(retries));
        }
        Log("Forced shutdown.");
    }
};