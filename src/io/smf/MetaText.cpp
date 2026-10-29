#include "io/smf/MetaText.h"
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#include <langinfo.h>
#endif

namespace
{
bool isValidUtf8(const char* data, int length)
{
    int i = 0;
    while (i < length)
    {
        auto byte = static_cast<unsigned char>(data[i]);
        int seqLen;

        if (byte <= 0x7F)
            seqLen = 1;
        else if ((byte & 0xE0) == 0xC0 && byte >= 0xC2)
            seqLen = 2;
        else if ((byte & 0xF0) == 0xE0)
            seqLen = 3;
        else if ((byte & 0xF8) == 0xF0 && byte <= 0xF4)
            seqLen = 4;
        else
            return false;

        if (i + seqLen > length)
            return false;

        for (int j = 1; j < seqLen; ++j)
            if ((static_cast<unsigned char>(data[i + j]) & 0xC0) != 0x80)
                return false;

        i += seqLen;
    }
    return true;
}
} // namespace

namespace MetaText
{
juce::String decode(const std::uint8_t* bytes, int length)
{
    auto* data = reinterpret_cast<const char*>(bytes);
    if (data == nullptr || length <= 0)
        return {};

    if (isValidUtf8(data, length))
        return juce::String::fromUTF8(data, length);

#ifdef _WIN32
    int wideLen = MultiByteToWideChar(CP_ACP, 0, data, length, nullptr, 0);
    if (wideLen > 0)
    {
        std::vector<wchar_t> wideBuf(wideLen + 1, L'\0');
        MultiByteToWideChar(CP_ACP, 0, data, length, wideBuf.data(), wideLen);
        return juce::String(wideBuf.data());
    }
#else
    const char* localeEncoding = nl_langinfo(CODESET);
    iconv_t cd = iconv_open("UTF-8", localeEncoding);
    if (cd != reinterpret_cast<iconv_t>(-1))
    {
        size_t inBytesLeft = static_cast<size_t>(length);
        size_t outBytesLeft = inBytesLeft * 4;
        std::vector<char> outBuf(outBytesLeft + 1, '\0');

        char* inPtr = const_cast<char*>(data);
        char* outPtr = outBuf.data();

        size_t result = iconv(cd, &inPtr, &inBytesLeft, &outPtr, &outBytesLeft);
        iconv_close(cd);

        if (result != static_cast<size_t>(-1))
        {
            int outLength = static_cast<int>(outPtr - outBuf.data());
            return juce::String::fromUTF8(outBuf.data(), outLength);
        }
    }
#endif

    return juce::String::fromUTF8(data, length);
}
} // namespace MetaText
