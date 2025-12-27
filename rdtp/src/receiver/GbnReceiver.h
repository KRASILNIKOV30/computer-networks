#pragma once
#include "../common/RdtSocket.h"
#include <fstream>
#include <map>

class GbnReceiver {
public:
    GbnReceiver(uint16_t port, const std::string& outfile, bool debug)
            : m_port(port), m_outfile(outfile), m_debug(debug) {}

    void Run() {
        m_socket.Bind(m_port);
        std::cout << "Receiver started on port " << m_port << ". Writing to " << m_outfile << std::endl;

        while (true) {
            Packet p;
            sockaddr_in senderAddr;
            m_socket.SetTimeout(0);

            if (m_socket.RecvFrom(p, &senderAddr)) {
                HandlePacket(p, senderAddr);
                if (m_finished) break;
            }
        }
    }

private:
    uint16_t m_port;
    std::string m_outfile;
    bool m_debug;
    RdtSocket m_socket;

    uint32_t m_expectedSeq = 0;
    bool m_handshakeDone = false;
    bool m_finished = false;
    std::ofstream m_fileStream;

    void Log(const std::string& msg) {
        if (m_debug) std::cout << "[RECEIVER] " << msg << std::endl;
    }

    void SendAck(uint32_t seq, uint8_t flags, const sockaddr_in& dest) {
        Packet ack;
        ack.header.seqNum = seq;
        ack.header.flags = static_cast<uint8_t>(PacketType::ACK) | flags;

        m_socket.SendTo(ack, dest);
        Log("Sent ACK #" + std::to_string(seq));
    }

    void HandlePacket(const Packet& p, const sockaddr_in& sender) {
        if (p.header.flags & static_cast<uint8_t>(PacketType::SYN)) {
            Log("Received SYN");
            m_expectedSeq = 1;
            m_handshakeDone = true;
            m_fileStream.open(m_outfile, std::ios::binary);
            SendAck(0, static_cast<uint8_t>(PacketType::SYN), sender);
            return;
        }

        if (p.header.flags & static_cast<uint8_t>(PacketType::FIN)) {
            Log("Received FIN");
            SendAck(p.header.seqNum, static_cast<uint8_t>(PacketType::FIN), sender);
            m_fileStream.close();
            m_finished = true;
            return;
        }

        if (p.header.flags & static_cast<uint8_t>(PacketType::DATA)) {
            Log("Received DATA #" + std::to_string(p.header.seqNum));

            if (!m_handshakeDone) {
                return;
            }

            if (p.header.seqNum == m_expectedSeq) {
                m_fileStream.write(reinterpret_cast<const char*>(p.payload.data()), p.payload.size());
                SendAck(m_expectedSeq, 0, sender);
                m_expectedSeq++;
            } else {
                Log("Unexpected SeqNum: " + std::to_string(p.header.seqNum) + " Expected: " + std::to_string(m_expectedSeq));
                if (m_expectedSeq > 0) {
                    SendAck(m_expectedSeq - 1, 0, sender);
                } else {
                    SendAck(0, static_cast<uint8_t>(PacketType::SYN), sender);
                }
            }
        }
    }
};
