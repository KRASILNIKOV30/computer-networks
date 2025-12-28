#pragma once
#include "../../lib/Connection.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

class SmtpClient
{
public:
    SmtpClient(const std::string& serverAddress, uint16_t port = 25)
            : m_connection(port, serverAddress)
    {
        ReceiveWelcomeMessage();
    }

    void SendEmail(
            const std::string& from,
            const std::string& to,
            const std::string& subject,
            const std::string& body)
    {
        SendGreeting();
        SendMailFrom(from);
        SendRcptTo(to);
        SendData();
        SendEmailContent(from, to, subject, body);
        SendQuit();
    }

private:
    Connection m_connection;
    static constexpr int SUCCESS_CODE = 250;
    static constexpr int SERVICE_READY_CODE = 220;
    static constexpr int START_DATA_CODE = 354;
    static constexpr int GOODBYE_CODE = 221;

    std::string ReadSmtpResponse()
    {
        std::string fullResponse;
        std::string line;

        do
        {
            line = m_connection.ReadLine();
            fullResponse += line;

            if (line.size() < 4) break;

            if (line[3] != '-')
            {
                break;
            }
        } while (true);

        return fullResponse;
    }

    void ReceiveWelcomeMessage()
    {
        const auto response = ReadSmtpResponse();
        if (!response.starts_with(std::to_string(SERVICE_READY_CODE)))
        {
            throw std::runtime_error("SMTP server did not respond with 220: " + response);
        }
    }

    void SendGreeting()
    {
        const std::string ehloCommand = "EHLO client.example.com\r\n";
        m_connection.Send(ehloCommand);

        auto response = ReadSmtpResponse();

        if (response.starts_with(std::to_string(SUCCESS_CODE)))
        {
            return;
        }

        std::cout << "EHLO failed. Falling back to HELO..." << std::endl;
        SendHelo();
    }

    void SendHelo()
    {
        const std::string heloCommand = "HELO client.example.com\r\n";
        m_connection.Send(heloCommand);
        const auto response = ReadSmtpResponse();
        if (!response.starts_with(std::to_string(SUCCESS_CODE)))
        {
            throw std::runtime_error("HELO failed: " + response);
        }
    }

    void SendMailFrom(const std::string& from)
    {
        const std::string mailFromCommand = "MAIL FROM: <" + from + ">\r\n";
        m_connection.Send(mailFromCommand);
        const auto response = ReadSmtpResponse();
        if (!response.starts_with(std::to_string(SUCCESS_CODE)))
        {
            throw std::runtime_error("MAIL FROM failed: " + response);
        }
    }

    void SendRcptTo(const std::string& to)
    {
        const std::string rcptToCommand = "RCPT TO: <" + to + ">\r\n";
        m_connection.Send(rcptToCommand);
        const auto response = ReadSmtpResponse();
        if (!response.starts_with(std::to_string(SUCCESS_CODE)))
        {
            throw std::runtime_error("RCPT TO failed: " + response);
        }
    }

    void SendData()
    {
        const std::string dataCommand = "DATA\r\n";
        m_connection.Send(dataCommand);
        const auto response = ReadSmtpResponse();
        if (!response.starts_with(std::to_string(START_DATA_CODE)))
        {
            throw std::runtime_error("DATA command failed: " + response);
        }
    }

    void SendEmailContent(
            const std::string& from,
            const std::string& to,
            const std::string& subject,
            const std::string& body)
    {
        std::string emailContent;
        emailContent += "From: " + from + "\r\n";
        emailContent += "To: " + to + "\r\n";
        emailContent += "Subject: " + subject + "\r\n";
        emailContent += "\r\n";
        emailContent += body + "\r\n";
        emailContent += ".\r\n";

        m_connection.Send(emailContent);
        const auto response = ReadSmtpResponse();
        if (!response.starts_with(std::to_string(SUCCESS_CODE)))
        {
            throw std::runtime_error("Failed to send email content: " + response);
        }
    }

    void SendQuit()
    {
        const std::string quitCommand = "QUIT\r\n";
        m_connection.Send(quitCommand);
        const auto response = ReadSmtpResponse();
        if (!response.starts_with(std::to_string(GOODBYE_CODE)))
        {
            throw std::runtime_error("QUIT failed: " + response);
        }
    }
};