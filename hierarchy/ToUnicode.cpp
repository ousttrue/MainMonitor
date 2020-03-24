#include "ToUnicode.h"
#include <windows.h>
#include <vector>
#include <system_error>

std::wstring ToUnicode(const std::string &src, UINT CP)
{
    auto const dest_size = ::MultiByteToWideChar(CP, 0U, src.data(), -1, nullptr, 0U);
    std::vector<wchar_t> dest(dest_size, L'\0');
    if (::MultiByteToWideChar(CP, 0U, src.data(), -1, dest.data(), (UINT)dest.size()) == 0)
    {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    dest.resize(std::char_traits<wchar_t>::length(dest.data()));
    dest.shrink_to_fit();
    return std::wstring(dest.begin(), dest.end());
}

std::wstring Utf8ToUnicode(const std::string &src)
{
    return ToUnicode(src, CP_UTF8);
}
