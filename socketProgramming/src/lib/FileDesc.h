#pragma once
#include <stdexcept>
#include <system_error>
#include <unistd.h>
#include <utility>

class FileDesc
{
    constexpr static int InvalidDesc = -1;

public:
    FileDesc() = default;

    explicit FileDesc(const int desc)
            : m_desc(desc == InvalidDesc || desc >= 0
                     ? desc
                     : throw std::invalid_argument("Invalid file descriptor"))
    {
    }

    FileDesc(const FileDesc&) = delete;
    FileDesc& operator=(const FileDesc&) = delete;

    FileDesc(FileDesc&& other) noexcept
            : m_desc(std::exchange(other.m_desc, InvalidDesc))
            {
            }

    FileDesc& operator=(FileDesc&& rhs)
    {
        if (this != &rhs)
        {
            Swap(rhs);
            rhs.Close();
        }
        return *this;
    }

    ~FileDesc()
    {
        try
        {
            Close();
        }
        catch (...)
        {
        }
    }

    void Swap(FileDesc& other)
    {
        std::swap(m_desc, other.m_desc);
    }

    [[nodiscard]] bool IsOpen() const noexcept
    {
        return m_desc != InvalidDesc;
    }

    void Close()
    {
        if (IsOpen())
        {
            if (close(m_desc) != 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
            m_desc = InvalidDesc;
        }
    }

    [[nodiscard]] int Get() const noexcept
    {
        return m_desc;
    }

    size_t Read(void* buffer, const size_t length)
    {
        EnsureOpen();
        if (const auto bytesRead = read(m_desc, buffer, length); bytesRead != -1)
        {
            return static_cast<size_t>(bytesRead);
        }
        throw std::system_error(errno, std::generic_category());
    }

private:
    void EnsureOpen() const
    {
        if (!IsOpen())
        {
            throw std::logic_error("File is not open");
        }
    }
    int m_desc = InvalidDesc;
};
