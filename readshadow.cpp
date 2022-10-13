#include <algorithm>
#include <string>
#include <vector>
#include <iostream>

#include <windows.h>

#define Log(fmt, ...)                                                                                                  \
    do                                                                                                                 \
    {                                                                                                                  \
        fprintf(stderr, fmt, __VA_ARGS__);                                                                             \
        fprintf(stderr, "\n");                                                                                         \
    } while (0)

std::string_view GetCmdOption(int argc, char* argv[], const std::string& option)
{
    for (int i = 0; i < argc; ++i)
    {
        std::string_view arg(argv[i]);
        auto pos = arg.find(option);
        if (pos != std::string_view::npos)
        {
            return i + 1 < argc ? argv[i + 1] : std::string_view();
        }
    }

    return {};
}

int Read(std::string disk, LARGE_INTEGER offset, std::vector<uint8_t>& buffer)
{
    HANDLE hDisk = CreateFileA(
        disk.c_str(),
        FILE_READ_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
        NULL);

    if (hDisk == INVALID_HANDLE_VALUE)
    {
        Log("Failed CreateFileA [0x%08x]", GetLastError());
        return 1;
    }

    LARGE_INTEGER newOffset;
    BOOL ret = SetFilePointerEx(hDisk, offset, &newOffset, FILE_BEGIN);
    if (!ret)
    {
        Log("Failed SetFilePointerEx [0x%08x]", GetLastError());
        return 1;
    }

    DWORD processed = 0;
    ret = ReadFile(hDisk, buffer.data(), static_cast<DWORD>(buffer.size()), &processed, NULL);
    if (!ret)
    {
        Log("Failed ReadFile [0x%08x]", GetLastError());
        return 1;
    }

    buffer.resize(processed);
    return 0;
}

void HexDump(LARGE_INTEGER diskOffset, const std::vector<uint8_t>& buffer)
{
    const auto hex = std::string_view("0123456789ABCDEF");
    const auto bytesPerLine = 16;

    if (buffer.empty())
    {
        return;
    }

    for (size_t offset = 0; offset < buffer.size() - 1; offset += bytesPerLine)
    {
        const auto length = (offset + bytesPerLine) <= buffer.size() ? bytesPerLine : buffer.size() % bytesPerLine;
        std::basic_string_view<uint8_t> line(buffer.data() + offset, length);

        std::cout << "0x" << std::hex << diskOffset.QuadPart + offset << "  ";

        for (auto c : line)
        {
            std::cout << hex[(c >> 4) & 0x0F] << hex[c & 0x0F] << " ";
        }

        std::cout << " ";
        for (auto c : line)
        {
            isprint(c) ? std::cout << c : std::cout << '.';
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    const auto disk = ::GetCmdOption(argc, argv, "disk");
    const auto offset = ::GetCmdOption(argc, argv, "offset");
    const auto length = ::GetCmdOption(argc, argv, "length");
    const auto preFill = ::GetCmdOption(argc, argv, "prefill");

    if (disk.empty() || offset.empty() || length.empty())
    {
        Log("%s -disk \\\\?\\GLOBALROOT\\Device\\HarddiskVolumeShadowCopy16 -offset 2048 -length 512 -prefill 0x41",
            argv[0]);
        return 1;
    }

    size_t pos;
    LARGE_INTEGER liOffset;
    liOffset.QuadPart = std::stoll(offset.data(), &pos, (offset.size() > 2 && offset[1] == 'x') ? 16 : 10);
    if (liOffset.QuadPart % 512 != 0)
    {
        Log("Invalid parameter 'offset' must be aligned to 512 bytes boundary");
    }

    const auto llength = std::stol(length.data(), &pos, (length.size() > 2 && length[1] == 'x') ? 16 : 10);
    if (llength % 512 != 0)
    {
        Log("Invalid parameter 'length' must be aligned to 512 bytes boundary");
    }

    if (preFill.size() > 1)
    {
        Log("Invalid parameter 'prefill' must be one character, only the first one will be use"); 
    }

    std::vector<uint8_t> buffer;
    buffer.resize(llength);
    if (!preFill.empty())
    {
        std::fill(std::begin(buffer), std::end(buffer), preFill[0]);
    }

    int status = Read(disk.data(), liOffset, buffer);
    if (status)
    {
        Log("Failed Read [0x%08x]", GetLastError());
        return status;
    }

    HexDump(liOffset, buffer);
    return 0;
}
