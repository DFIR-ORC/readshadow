#include <algorithm>
#include <string>
#include <vector>
#include <iostream>
#include <numeric>

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

int main(int argc, char* argv[])
{
    const auto disk = ::GetCmdOption(argc, argv, "disk");
    const auto block_size = ::GetCmdOption(argc, argv, "block_size");

    if (disk.empty() || block_size.empty())
    {
        // \\?\GLOBALROOT\Device\HarddiskVolumeShadowCopy6
        Log("%s -disk \\\\?\\GLOBALROOT\\Device\\HarddiskVolumeShadowCopy6 -block_size 512", argv[0]);
        return 1;
    }

    size_t pos;
    const auto bufferSize =
        std::stol(block_size.data(), &pos, (block_size.size() > 2 && block_size[1] == 'x') ? 16 : 10);
    if (bufferSize % 512 != 0 || bufferSize == 0)
    {
        Log("Invalid parameter 'block_size' must be aligned to 512 bytes boundary");
    }

    HANDLE hDisk = CreateFileA(
        disk.data(),
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

    std::vector<uint8_t> buffer;
    std::vector<uint8_t> referenceBuffer;
    buffer.resize(bufferSize);
    uint64_t cbTotalRead = 0;

    bool exit = false;
    while (!exit)
    {
        // fill buffer with a range of sequentially increasing value: 0x00, 0x01, 0x02...
        std::iota(std::begin(buffer), std::end(buffer), 0);
        *((uint32_t*)&buffer[0]) = 0x0C0FFEE;

        referenceBuffer = buffer;

        DWORD dwRead = 0;
        BOOL ret = ReadFile(hDisk, buffer.data(), static_cast<DWORD>(buffer.size()), &dwRead, NULL);
        if (!ret)
        {
            Log("Failed ReadFile (offset: %llx) [0x%08x]", cbTotalRead, GetLastError());
            return 1;
        }

        if (dwRead != buffer.size())
        {
            exit = true;
        }

        if (referenceBuffer == buffer)
        {
            printf("Cow block probably missing: 0x%llx\n", cbTotalRead);
        }

        cbTotalRead += dwRead;
    }

    return 0;
}
