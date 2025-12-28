#pragma once
#include <string>
#include <regex>
#include <iostream>

struct ParsedRequest {
    std::string method;
    std::string host;
    int port = 80;
    std::string path;
    std::string version;
    bool isValid = false;
    std::string fullUrl;
};

class HttpUtils {
public:
    static ParsedRequest ParseRequest(const std::string& rawRequest) {
        ParsedRequest req;
        std::istringstream stream(rawRequest);
        std::string url;

        stream >> req.method >> url >> req.version;

        if (req.method != "GET") {
            return req;
        }

        req.fullUrl = url;

        std::regex urlRegex(R"(http://([^/:]+)(?::(\d+))?(/.*)?)");
        std::smatch match;

        if (std::regex_search(url, match, urlRegex)) {
            req.host = match[1];
            if (match[2].matched) {
                req.port = std::stoi(match[2]);
            }
            req.path = match[3].matched ? match[3].str() : "/";
            req.isValid = true;
        } else {
            std::cerr << "Unsupported URL format: " << url << std::endl;
        }

        return req;
    }
};