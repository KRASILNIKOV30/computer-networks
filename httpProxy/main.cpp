#include "../lib/Acceptor.h"
#include "../lib/Socket.h"
#include "UpstreamConnection.h"
#include "HttpUtils.h"
#include "HttpCache.h"
#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

std::mutex logMutex;
HttpCache cache;

void Log(const std::string& msg) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::cout << "[Proxy] " << msg << std::endl;
}

void HandleClient(Socket clientSocket) {
    try {
        char buffer[8192];
        size_t bytesRead = clientSocket.Read(buffer, sizeof(buffer) - 1);
        if (bytesRead == 0) return;

        buffer[bytesRead] = '\0';
        std::string rawRequest(buffer);

        ParsedRequest req = HttpUtils::ParseRequest(rawRequest);

        if (!req.isValid) {
            Log("Invalid or unsupported request: " + rawRequest.substr(0, rawRequest.find('\n')));
            return;
        }

        Log("Request: " + req.method + " " + req.fullUrl);

        if (cache.Has(req.fullUrl)) {
            Log("Cache HIT: " + req.fullUrl);
            cache.ServeFromCache(req.fullUrl, [&](const char* data, size_t len) {
                clientSocket.Send(data, len, 0);
            });
            return;
        }

        Log("Cache MISS: Fetching from " + req.host);

        try {
            UpstreamConnection server(req.host, req.port);

            std::string newRequest = "GET " + req.path + " " + req.version + "\r\n";
            newRequest += "Host: " + req.host + "\r\n";
            newRequest += "Connection: close\r\n\r\n";

            server.Send(newRequest);

            auto cacheFile = cache.OpenWrite(req.fullUrl);

            char remoteBuf[4096];
            size_t remoteRead;
            size_t totalBytes = 0;

            while ((remoteRead = server.Receive(remoteBuf, sizeof(remoteBuf))) > 0) {
                clientSocket.Send(remoteBuf, remoteRead, 0);
                if (cacheFile.is_open()) {
                    cacheFile.write(remoteBuf, remoteRead);
                }
                totalBytes += remoteRead;
            }

            Log("Completed: " + req.fullUrl + " (" + std::to_string(totalBytes) + " bytes)");

        } catch (const std::exception& e) {
            Log("Error fetching from upstream: " + std::string(e.what()));
            std::string errorResponse = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
            clientSocket.Send(errorResponse.data(), errorResponse.size(), 0);
        }

    } catch (const std::exception& e) {
        Log("Client handler error: " + std::string(e.what()));
    }
}

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    try {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        Acceptor acceptor(addr, 10);
        Log("Proxy Server started on port " + std::to_string(port));
        Log("Cache directory: ./cache");

        while (true) {
            Socket client = acceptor.Accept();
            std::thread(HandleClient, std::move(client)).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}