// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerLogging.h"
#include "icu.h"

namespace AppInstaller::Utility
{
    // Same as std::isspace(char)
#define AICLI_SPACE_CHARS " \f\n\r\t\v"sv

    using namespace std::string_view_literals;
    constexpr std::string_view s_SpaceChars = AICLI_SPACE_CHARS;
    constexpr std::wstring_view s_WideSpaceChars = L"" AICLI_SPACE_CHARS;

    namespace
    {
        // Contains the ICU objects necessary to do break iteration.
        struct ICUBreakIterator
        {
            ICUBreakIterator(std::string_view input, UBreakIteratorType type)
            {
                UErrorCode err = U_ZERO_ERROR;

                m_text.reset(utext_openUTF8(nullptr, input.data(), wil::safe_cast<int64_t>(input.length()), &err));
                if (U_FAILURE(err))
                {
                    AICLI_LOG(Core, Error, << "utext_openUTF8 returned " << err);
                    THROW_HR(APPINSTALLER_CLI_ERROR_ICU_BREAK_ITERATOR_ERROR);
                }

                m_brk.reset(ubrk_open(type, nullptr, nullptr, 0, &err));
                if (U_FAILURE(err))
                {
                    AICLI_LOG(Core, Error, << "ubrk_open returned " << err);
                    THROW_HR(APPINSTALLER_CLI_ERROR_ICU_BREAK_ITERATOR_ERROR);
                }

                ubrk_setUText(m_brk.get(), m_text.get(), &err);
                if (U_FAILURE(err))
                {
                    AICLI_LOG(Core, Error, << "ubrk_setUText returned " << err);
                    THROW_HR(APPINSTALLER_CLI_ERROR_ICU_BREAK_ITERATOR_ERROR);
                }

                int32_t i = ubrk_first(m_brk.get());
                if (i != 0)
                {
                    AICLI_LOG(Core, Error, << "ubrk_first returned " << i);
                    THROW_HR(APPINSTALLER_CLI_ERROR_ICU_BREAK_ITERATOR_ERROR);
                }
            }

            // Gets the current break value; the byte offset or UBRK_DONE.
            int32_t CurrentBreak() const { return m_currentBrk; }

            // Gets the current byte offset, throwing if the value is UBRK_DONE or negative.
            size_t CurrentOffset() const
            {
                THROW_HR_IF(E_NOT_VALID_STATE, m_currentBrk < 0);
                return static_cast<size_t>(m_currentBrk);
            }

            // Returns the byte offset of the next break in the string
            int32_t Next()
            {
                m_currentBrk = ubrk_next(m_brk.get());
                return m_currentBrk;
            }

            // Returns the byte offset of the next count'th break in the string
            int32_t Advance(size_t count)
            {
                for (size_t i = 0; i < count && m_currentBrk != UBRK_DONE; ++i)
                {
                    Next();
                }
                return m_currentBrk;
            }

            // Returns code point of the character at m_currentBrk, or U_SENTINEL if m_currentBrk points to the end.
            UChar32 CurrentCodePoint()
            {
                return utext_char32At(m_text.get(), m_currentBrk);
            }

        private:
            wil::unique_any<UText*, decltype(utext_close), &utext_close> m_text;
            wil::unique_any<UBreakIterator*, decltype(ubrk_close), &ubrk_close> m_brk;
            int32_t m_currentBrk = 0;
        };
    }

    bool CaseInsensitiveEquals(std::string_view a, std::string_view b)
    {
        return ToLower(a) == ToLower(b);
    }

    bool CaseInsensitiveStartsWith(std::string_view a, std::string_view b)
    {
        return a.length() >= b.length() && CaseInsensitiveEquals(a.substr(0, b.length()), b);
    }

    bool ICUCaseInsensitiveEquals(std::string_view a, std::string_view b)
    {
        return FoldCase(a) == FoldCase(b);
    }

    bool ICUCaseInsensitiveStartsWith(std::string_view a, std::string_view b)
    {
        return a.length() >= b.length() && ICUCaseInsensitiveEquals(a.substr(0, b.length()), b);
    }

    std::string ConvertToUTF8(std::wstring_view input)
    {
        if (input.empty())
        {
            return {};
        }

        int utf8ByteCount = WideCharToMultiByte(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), nullptr, 0, nullptr, nullptr);
        THROW_LAST_ERROR_IF(utf8ByteCount == 0);

        // Since the string view should not contain the null char, the result won't either.
        // This allows us to use the resulting size value directly in the string constructor.
        std::string result(wil::safe_cast<size_t>(utf8ByteCount), '\0');

        int utf8BytesWritten = WideCharToMultiByte(CP_UTF8, 0, input.data(), wil::safe_cast<int>(input.length()), &result[0], wil::safe_cast<int>(result.size()), nullptr, nullptr);
        FAIL_FAST_HR_IF(E_UNEXPECTED, utf8ByteCount != utf8BytesWritten);

        return result;
    }

    std::wstring ConvertToUTF16(std::string_view input, UINT codePage)
    {
        if (input.empty())
        {
            return {};
        }

        int utf16CharCount = MultiByteToWideChar(codePage, 0, input.data(), wil::safe_cast<int>(input.length()), nullptr, 0);
        THROW_LAST_ERROR_IF(utf16CharCount == 0);

        // Since the string view should not contain the null char, the result won't either.
        // This allows us to use the resulting size value directly in the string constructor.
        std::wstring result(wil::safe_cast<size_t>(utf16CharCount), L'\0');

        int utf16CharsWritten = MultiByteToWideChar(codePage, 0, input.data(), wil::safe_cast<int>(input.length()), &result[0], wil::safe_cast<int>(result.size()));
        FAIL_FAST_HR_IF(E_UNEXPECTED, utf16CharCount != utf16CharsWritten);

        return result;
    }

    size_t UTF8Length(std::string_view input)
    {
        ICUBreakIterator itr{ input, UBRK_CHARACTER };

        size_t numGraphemeClusters = 0;

        while (itr.Next() != UBRK_DONE)
        {
            numGraphemeClusters++;
        }

        return numGraphemeClusters;
    }

    size_t UTF8ColumnWidth(const NormalizedUTF8<NormalizationC>& input)
    {
        ICUBreakIterator itr{ input, UBRK_CHARACTER };

        size_t columnWidth = 0;
        UChar32 currentCP = 0;

        currentCP = itr.CurrentCodePoint();
        while (itr.Next() != UBRK_DONE && currentCP != U_SENTINEL)
        {
            int32_t width = u_getIntPropertyValue(currentCP, UCHAR_EAST_ASIAN_WIDTH);
            columnWidth += width == U_EA_FULLWIDTH || width == U_EA_WIDE ? 2 : 1;

            currentCP = itr.CurrentCodePoint();
        }

        return columnWidth;
    }

    std::string_view UTF8Substring(std::string_view input, size_t offset, size_t count)
    {
        ICUBreakIterator itr{ input, UBRK_CHARACTER };

        // Offset was past end, throw just like std::string::substr
        if (itr.Advance(offset) == UBRK_DONE)
        {
            throw std::out_of_range("UTF8Substring: offset past end of input");
        }

        size_t utf8Offset = itr.CurrentOffset();
        size_t utf8Count = 0;

        // Count past end, convert to npos to get all of string
        if (itr.Advance(count) == UBRK_DONE)
        {
            utf8Count = std::string_view::npos;
        }
        else
        {
            utf8Count = itr.CurrentOffset() - utf8Offset;
        }

        return input.substr(utf8Offset, utf8Count);
    }

    std::string UTF8TrimRightToColumnWidth(const NormalizedUTF8<NormalizationC>& input, size_t expectedWidth, size_t& actualWidth)
    {
        ICUBreakIterator itr{ input, UBRK_CHARACTER };

        size_t columnWidth = 0;
        UChar32 currentCP = 0;
        int32_t currentBrk = 0;
        int32_t nextBrk = 0;

        currentCP = itr.CurrentCodePoint();
        currentBrk = itr.CurrentBreak();
        nextBrk = itr.Next();
        while (nextBrk != UBRK_DONE && currentCP != U_SENTINEL)
        {
            int32_t width = u_getIntPropertyValue(currentCP, UCHAR_EAST_ASIAN_WIDTH);
            int charWidth = width == U_EA_FULLWIDTH || width == U_EA_WIDE ? 2 : 1;
            columnWidth += charWidth;

            if (columnWidth > expectedWidth)
            {
                columnWidth -= charWidth;
                break;
            }

            currentCP = itr.CurrentCodePoint();
            currentBrk = nextBrk;
            nextBrk = itr.Next();
        }

        actualWidth = columnWidth;

        return input.substr(0, currentBrk);
    }

    std::string Normalize(std::string_view input, NORM_FORM form)
    {
        if (input.empty())
        {
            return {};
        }

        return ConvertToUTF8(Normalize(ConvertToUTF16(input), form));
    }

    std::wstring Normalize(std::wstring_view input, NORM_FORM form)
    {
        if (input.empty())
        {
            return {};
        }

        std::wstring result;

        int cchEstimate = NormalizeString(form, input.data(), static_cast<int>(input.length()), NULL, 0);
        for (;;)
        {
            result.resize(cchEstimate);
            cchEstimate = NormalizeString(form, input.data(), static_cast<int>(input.length()), &result[0], cchEstimate);

            if (cchEstimate > 0)
            {
                result.resize(cchEstimate);
                return result;
            }
            else
            {
                DWORD dwError = GetLastError();
                THROW_LAST_ERROR_IF(dwError != ERROR_INSUFFICIENT_BUFFER);

                // New guess is negative of the return value.
                cchEstimate = -cchEstimate;

                THROW_HR_IF_MSG(E_UNEXPECTED, static_cast<size_t>(cchEstimate) <= result.size(), "New estimate should never be less than previous value");
            }
        }
    }

    std::string ToLower(std::string_view in)
    {
        std::string result(in);
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return result;
    }

    std::wstring ToLower(std::wstring_view in)
    {
        std::wstring result(in);
        std::transform(result.begin(), result.end(), result.begin(),
            [](unsigned short c) { return std::towlower(c); });
        return result;
    }

    std::string FoldCase(std::string_view input)
    {
        if (input.empty())
        {
            return {};
        }

        wil::unique_any<UCaseMap*, decltype(ucasemap_close), &ucasemap_close> caseMap;
        UErrorCode errorCode = UErrorCode::U_ZERO_ERROR;
        caseMap.reset(ucasemap_open(nullptr, U_FOLD_CASE_DEFAULT, &errorCode));

        if (U_FAILURE(errorCode))
        {
            AICLI_LOG(Core, Error, << "ucasemap_open returned " << errorCode);
            THROW_HR(APPINSTALLER_CLI_ERROR_ICU_CASEMAP_ERROR);
        }

        int32_t cch = ucasemap_utf8FoldCase(caseMap.get(), nullptr, 0, input.data(), static_cast<int32_t>(input.size()), &errorCode);
        if (errorCode != U_BUFFER_OVERFLOW_ERROR)
        {
            AICLI_LOG(Core, Error, << "ucasemap_utf8FoldCase returned " << errorCode);
            THROW_HR(APPINSTALLER_CLI_ERROR_ICU_CASEMAP_ERROR);
        }

        errorCode = UErrorCode::U_ZERO_ERROR;

        std::string result(cch, '\0');
        cch = ucasemap_utf8FoldCase(caseMap.get(), &result[0], cch, input.data(), static_cast<int32_t>(input.size()), &errorCode);
        if (U_FAILURE(errorCode))
        {
            AICLI_LOG(Core, Error, << "ucasemap_utf8FoldCase returned " << errorCode);
            THROW_HR(APPINSTALLER_CLI_ERROR_ICU_CASEMAP_ERROR);
        }

        while (result.back() == '\0')
        {
            result.pop_back();
        }

        return result;
    }

    NormalizedString FoldCase(const NormalizedString& input)
    {
        NormalizedString result;
        result.assign(FoldCase(static_cast<std::string_view>(input)));
        return result;
    }

    bool IsEmptyOrWhitespace(std::string_view str)
    {
        if (str.empty())
        {
            return true;
        }

        return str.find_last_not_of(s_SpaceChars) == std::string_view::npos;
    }

    bool IsEmptyOrWhitespace(std::wstring_view str)
    {
        if (str.empty())
        {
            return true;
        }

        return str.find_last_not_of(s_WideSpaceChars) == std::wstring_view::npos;
    }

    bool FindAndReplace(std::string& inputStr, std::string_view token, std::string_view value)
    {
        bool result = false;
        std::string::size_type pos = 0u;
        while ((pos = inputStr.find(token, pos)) != std::string::npos)
        {
            result = true;
            inputStr.replace(pos, token.length(), value);
            pos += value.length();
        }
        return result;
    }

    std::string& Trim(std::string& str)
    {
        if (!str.empty())
        {
            size_t begin = str.find_first_not_of(s_SpaceChars);
            size_t end = str.find_last_not_of(s_SpaceChars);

            if (begin == std::string_view::npos || end == std::string_view::npos)
            {
                str.clear();
            }
            else if (begin != 0 || end != str.length() - 1)
            {
                str = str.substr(begin, (end - begin) + 1);
            }
        }

        return str;
    }

    std::wstring& Trim(std::wstring& str)
    {
        if (!str.empty())
        {
            size_t begin = str.find_first_not_of(s_WideSpaceChars);
            size_t end = str.find_last_not_of(s_WideSpaceChars);

            if (begin == std::string_view::npos || end == std::string_view::npos)
            {
                str.clear();
            }
            else if (begin != 0 || end != str.length() - 1)
            {
                str = str.substr(begin, (end - begin) + 1);
            }
        }

        return str;
    }

    std::string Trim(std::string&& str)
    {
        std::string result = std::move(str);
        Utility::Trim(result);
        return result;
    }

    std::string ReadEntireStream(std::istream& stream)
    {
        std::streampos currentPos = stream.tellg();
        stream.seekg(0, std::ios_base::end);

        auto offset = stream.tellg() - currentPos;
        stream.seekg(currentPos);

        // Don't allow use of this API for reading very large streams.
        THROW_HR_IF(E_OUTOFMEMORY, offset > static_cast<std::streamoff>(std::numeric_limits<uint32_t>::max()));
        std::string result(static_cast<size_t>(offset), '\0');
        stream.read(&result[0], offset);

        return result;
    }

    std::wstring ExpandEnvironmentVariables(const std::wstring& input)
    {
        if (input.empty())
        {
            return {};
        }

        DWORD charCount = ExpandEnvironmentStringsW(input.c_str(), nullptr, 0);
        THROW_LAST_ERROR_IF(charCount == 0);

        std::wstring result(wil::safe_cast<size_t>(charCount), L'\0');

        DWORD charCountWritten = ExpandEnvironmentStringsW(input.c_str(), &result[0], charCount);
        THROW_HR_IF(E_UNEXPECTED, charCount != charCountWritten);

        if (result.back() == L'\0')
        {
            result.resize(result.size() - 1);
        }

        return result;
    }
}
