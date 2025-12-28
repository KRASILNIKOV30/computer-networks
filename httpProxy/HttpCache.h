#pragma once
#include <string>
#include <fstream>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

class HttpCache {
public:
    HttpCache() {
        if (!fs::exists(m_cacheDir)) {
            fs::create_directory(m_cacheDir);
        }
    }

    std::string GetCacheFilePath(const std::string& url) const {
        std::hash<std::string> hasher;
        size_t hash = hasher(url);
        return (m_cacheDir / std::to_string(hash)).string();
    }

    bool Has(const std::string& url) const {
        return fs::exists(GetCacheFilePath(url));
    }

    void ServeFromCache(const std::string& url, std::function<void(const char*, size_t)> sendCallback) {
        std::string path = GetCacheFilePath(url);
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return;

        char buffer[4096];
        while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
            sendCallback(buffer, file.gcount());
        }
    }

    std::ofstream OpenWrite(const std::string& url) {
        return std::ofstream(GetCacheFilePath(url), std::ios::binary);
    }

private:
    fs::path m_cacheDir = "./cache";
};