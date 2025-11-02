#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include "../../lib/Acceptor.h"

constexpr int PORT = 8080;
const std::string WEB_ROOT = "www";

std::string ParseRequestPath(const std::string& request_str)
{
    std::istringstream request_stream(request_str);
    std::string method, path;
    request_stream >> method >> path;

    if (method != "GET") {
        return ""; // Поддерживаем только GET
    }

    if (path == "/") {
        path = "/index.html";
    }

    if (path.find("..") != std::string::npos) {
        return "";
    }

    return path;
}

std::string GetMimeType(const std::string& path)
{
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".css")) return "text/css";
    if (path.ends_with(".js")) return "application/javascript";
    if (path.ends_with(".jpg") || path.ends_with(".jpeg")) return "image/jpeg";
    if (path.ends_with(".png")) return "image/png";
    return "application/octet-stream";
}

std::vector<char> ReadFile(const std::filesystem::path& file_path)
{
    std::ifstream file(file_path, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Could not read file");
    }

    return buffer;
}

int main()
{
    try
    {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        Acceptor acceptor(server_addr, 10);
        std::cout << "Server started on port " << PORT << ". Waiting for connections..." << std::endl;
        std::cout << "Serving files from directory: '" << WEB_ROOT << "'" << std::endl;

        while (true)
        {
            try
            {
                Socket clientSocket = acceptor.Accept();
                std::cout << "\n--- New connection accepted ---" << std::endl;

                char buffer[4096];
                size_t bytesRead = clientSocket.Read(buffer, sizeof(buffer) - 1);
                buffer[bytesRead] = '\0';
                std::string requestStr(buffer);

                std::string path = ParseRequestPath(requestStr);
                if (path.empty()) {
                    std::cout << "Invalid or unsupported request." << std::endl;
                    continue;
                }

                std::filesystem::path filePath = WEB_ROOT + path;
                std::cout << "Requested file: " << filePath << std::endl;

                std::stringstream response;

                try
                {
                    auto fileContent = ReadFile(filePath);
                    std::string mimeType = GetMimeType(filePath.string());

                    response << "HTTP/1.1 200 OK\r\n";
                    response << "Content-Type: " << mimeType << "\r\n";
                    response << "Content-Length: " << fileContent.size() << "\r\n";
                    response << "\r\n";

                    clientSocket.Send(response.str().c_str(), response.str().length(), 0);
                    clientSocket.Send(fileContent.data(), fileContent.size(), 0);

                    std::cout << "Sent response: 200 OK, " << fileContent.size() << " bytes" << std::endl;
                }
                catch (const std::runtime_error& e)
                {
                    std::cerr << "Error: " << e.what() << " for file " << filePath << std::endl;

                    std::string body = "File Not Found";
                    response << "HTTP/1.1 404 Not Found\r\n";
                    response << "Content-Type: text/plain\r\n";
                    response << "Content-Length: " << body.length() << "\r\n";
                    response << "\r\n";
                    response << body;

                    clientSocket.Send(response.str().c_str(), response.str().length(), 0);
                    std::cout << "Sent response: 404 Not Found" << std::endl;
                }

                std::cout << "--- Connection closed ---" << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error handling client: " << e.what() << std::endl;
            }
        }
    }
    catch (const std::system_error& e)
    {
        std::cerr << "Server startup failed: " << e.what() << " (code: " << e.code() << ")" << std::endl;
        return 1;
    }

    return 0;
}