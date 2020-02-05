// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    std::string ConvertToUTF8(std::wstring_view input)
    {
        int utf8ByteCount = WideCharToMultiByte(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), nullptr, 0, nullptr, nullptr);
        THROW_LAST_ERROR_IF(utf8ByteCount == 0);

        // Since the string view should not contain the null char, the result won't either.
        // This allows us to use the resulting size value directly in the string constructor.
        std::string result(wil::safe_cast<size_t>(utf8ByteCount), '\0');

        int utf8BytesWritten = WideCharToMultiByte(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), &result[0], wil::safe_cast<int>(result.size()), nullptr, nullptr);
        FAIL_FAST_HR_IF(E_UNEXPECTED, utf8ByteCount != utf8BytesWritten);

        return result;
    }

    std::wstring ConvertToUTF16(std::string_view input)
    {
        int utf16CharCount = MultiByteToWideChar(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), nullptr, 0);
        THROW_LAST_ERROR_IF(utf16CharCount == 0);

        // Since the string view should not contain the null char, the result won't either.
        // This allows us to use the resulting size value directly in the string constructor.
        std::wstring result(wil::safe_cast<size_t>(utf16CharCount), L'\0');

        int utf16CharsWritten = MultiByteToWideChar(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), &result[0], wil::safe_cast<int>(result.size()));
        FAIL_FAST_HR_IF(E_UNEXPECTED, utf16CharCount != utf16CharsWritten);

        return result;
    }

    std::string ToLower(const std::string& in)
    {
        std::string result(in);
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    std::wstring ToLower(const std::wstring& in)
    {
        std::wstring result(in);
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned short c) { return std::towlower(c); });
        return result;
    }

    bool IsEmptyOrWhitespace(std::wstring_view str)
    {
        if (str.empty())
        {
            return true;
        }

        std::wstring inputAsWStr(str.data());
        bool nonWhitespaceNotFound = inputAsWStr.find_last_not_of(L" \t\v\f") == std::wstring::npos;

        return nonWhitespaceNotFound;
    }
}
