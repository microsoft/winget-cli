// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <filesystem>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>
#include <winget/LocIndependent.h>

namespace AppInstaller::Utility
{
    // Converts the given UTF16 string to UTF8
    std::string ConvertToUTF8(std::wstring_view input);

    // Converts the given UTF8 string to UTF16
    std::wstring ConvertToUTF16(std::string_view input, UINT codePage = CP_UTF8);

    // Tries to convert the given UTF8 string to UTF16
    std::optional<std::wstring> TryConvertToUTF16(std::string_view input, UINT codePage = CP_UTF8);

    // Converts the given UTF8 string to UTF32
    std::u32string ConvertToUTF32(std::string_view input);

    // Normalizes a UTF8 string to the given form.
    std::string Normalize(std::string_view input, NORM_FORM form = NORM_FORM::NormalizationKC);

    // Normalizes a UTF16 string to the given form.
    std::wstring Normalize(std::wstring_view input, NORM_FORM form = NORM_FORM::NormalizationKC);

    // Replaces any embedded null character found within the string.
    void ReplaceEmbeddedNullCharacters(std::string& s, char c = ' ');

    // Type to hold and force a normalized UTF8 string.
    // Also enables further normalization by replacing embedded null characters with spaces.
    template <NORM_FORM Form = NORM_FORM::NormalizationKC, bool ConvertEmbeddedNullToSpace = false>
    struct NormalizedUTF8 : public std::string
    {
        NormalizedUTF8() = default;

        template <size_t Size>
        NormalizedUTF8(const char(&s)[Size]) { AssignValue(std::string_view{ s, (s[Size - 1] == '\0' ? Size - 1 : Size) }); }

        NormalizedUTF8(std::string_view sv) { AssignValue(sv); }

        NormalizedUTF8(std::string& s) { AssignValue(s); }
        NormalizedUTF8(const std::string& s) { AssignValue(s); }
        NormalizedUTF8(std::string&& s) { AssignValue(s); }

        NormalizedUTF8(std::wstring_view sv) { AssignValue(sv); }

        NormalizedUTF8(const NormalizedUTF8& other) = default;
        NormalizedUTF8& operator=(const NormalizedUTF8& other) = default;

        NormalizedUTF8(NormalizedUTF8&& other) = default;
        NormalizedUTF8& operator=(NormalizedUTF8&& other) = default;

        template <size_t Size>
        NormalizedUTF8& operator=(const char(&s)[Size])
        {
            AssignValue(std::string_view{ s, (s[Size - 1] == '\0' ? Size - 1 : Size) });
            return *this;
        }

        NormalizedUTF8& operator=(std::string_view sv)
        {
            AssignValue(sv);
            return *this;
        }

        NormalizedUTF8& operator=(const std::string& s)
        {
            AssignValue(s);
            return *this;
        }

        NormalizedUTF8& operator=(std::string&& s)
        {
            AssignValue(s);
            return *this;
        }

    private:
        void AssignValue(std::string_view sv)
        {
            assign(Normalize(sv, Form));

            if constexpr (ConvertEmbeddedNullToSpace)
            {
                ReplaceEmbeddedNullCharacters(*this);
            }
        }

        void AssignValue(std::wstring_view sv)
        {
            assign(ConvertToUTF8(Normalize(sv, Form)));

            if constexpr (ConvertEmbeddedNullToSpace)
            {
                ReplaceEmbeddedNullCharacters(*this);
            }
        }
    };

    using NormalizedString = NormalizedUTF8<NORM_FORM::NormalizationKC, true>;

    // Compares the two UTF8 strings in a case-insensitive manner.
    // Use this if one of the values is a known value, and thus ToLower is sufficient.
    bool CaseInsensitiveEquals(std::string_view a, std::string_view b);

    // Compares the two UTF16 strings in a case-insensitive manner.
    // Use this if one of the values is a known value, and thus ToLower is sufficient.
    bool CaseInsensitiveEquals(std::wstring_view a, std::wstring_view b);

    // Returns if a UTF8 string is contained within a vector in a case-insensitive manner.
    bool CaseInsensitiveContains(const std::vector<std::string_view>& a, std::string_view b);

    // Determines if string a starts with string b.
    // Use this if one of the values is a known value, and thus ToLower is sufficient.
    bool CaseInsensitiveStartsWith(std::string_view a, std::string_view b);

    // Determines if string a contains string b.
    // Use this if one of the values is a known value, and thus ToLower is sufficient.
    bool CaseInsensitiveContainsSubstring(std::string_view a, std::string_view b);

    // Compares the two UTF8 strings in a case-insensitive manner, using ICU for case folding.
    bool ICUCaseInsensitiveEquals(std::string_view a, std::string_view b);

    // Determines if string a starts with string b, using ICU for case folding.
    bool ICUCaseInsensitiveStartsWith(std::string_view a, std::string_view b);

    // Returns the number of grapheme clusters (characters) in an UTF8-encoded string.
    size_t UTF8Length(std::string_view input);

    // Returns the number of units the UTF8-encoded string will take in terminal output. Some characters take 2 units in terminal output.
    size_t UTF8ColumnWidth(const NormalizedUTF8<NormalizationC>& input);

    // Returns a substring view in an UTF8-encoded string. Offset and count are measured in grapheme clusters (characters).
    std::string_view UTF8Substring(std::string_view input, size_t offset, size_t count);

    // Returns a substring view in an UTF8-encoded string trimmed to be at most expected length. Length is measured as units taken in terminal output.
    // Note the returned substring view might be less than specified length as some characters might take 2 units in terminal output.
    std::string UTF8TrimRightToColumnWidth(const NormalizedUTF8<NormalizationC>&, size_t expectedWidth, size_t& actualWidth);

    // Get the lower case version of the given std::string
    std::string ToLower(std::string_view in);

    // Get the lower case version of the given std::wstring
    std::wstring ToLower(std::wstring_view in);

    // Folds the case of the given std::string
    // See https://unicode-org.github.io/icu/userguide/transforms/casemappings.html#case-folding
    std::string FoldCase(std::string_view input);

    // Folds the case of the given NormalizedString, returning it as also Normalized
    // See https://unicode-org.github.io/icu/userguide/transforms/casemappings.html#case-folding
    NormalizedString FoldCase(const NormalizedString& input);

    // Checks if the input string is empty or whitespace
    bool IsEmptyOrWhitespace(std::string_view str);
    bool IsEmptyOrWhitespace(std::wstring_view str);

    // Find token in the input string and replace with value.
    // Returns a value indicating whether a replacement occurred.
    bool FindAndReplace(std::string& inputStr, std::string_view token, std::string_view value);

    // Replaces the token in the input string with value while copying to a new result.
    std::wstring ReplaceWhileCopying(std::wstring_view input, std::wstring_view token, std::wstring_view value);

    // Removes whitespace from the beginning and end of the string.
    std::string& Trim(std::string& str);
    std::string Trim(std::string&& str);

    // Removes whitespace from the beginning and end of the string.
    std::wstring& Trim(std::wstring& str);

    // Reads the entire stream into a string.
    std::string ReadEntireStream(std::istream& stream);

    // Reads the entire stream into a byte array.
    std::vector<std::uint8_t> ReadEntireStreamAsByteArray(std::istream& stream);

    // Expands environment variables within the input.
    std::wstring ExpandEnvironmentVariables(const std::wstring& input);

    // Converts the candidate path part into one suitable for the actual file system
    std::string MakeSuitablePathPart(std::string_view candidate);

    // Splits the file name part off of the given URI.
    std::pair<std::string, std::filesystem::path> SplitFileNameFromURI(std::string_view uri);

    // Gets the file name part of the given URI.
    std::filesystem::path GetFileNameFromURI(std::string_view uri);

    // Splits the string into words.
    std::vector<std::string> SplitIntoWords(std::string_view input);

    // Splits the string into lines.
    // Drops empty lines.
    std::vector<std::string> SplitIntoLines(std::string_view input, size_t maximum = 0);

    // Removes lines from the vector (and/or characters from the last line) so that it contains the maximum number of lines.
    // Returns true if changes were made, false if not.
    bool LimitOutputLines(std::vector<std::string>& lines, size_t lineWidth, size_t maximum);

    // Converts a container to a string representation of it.
    template <typename T, typename U>
    std::string ConvertContainerToString(const T& container, U toString)
    {
        std::ostringstream strstr;
        strstr << '[';

        bool firstItem = true;
        for (const auto& item : container)
        {
            if (firstItem)
            {
                firstItem = false;
            }
            else
            {
                strstr << ", ";
            }

            strstr << toString(item);
        }

        strstr << ']';
        return strstr.str();  // We need C++20 to get std::move(strstr).str() to extract the string from inside the stream
    }

    template <typename T>
    std::string ConvertContainerToString(const T& container)
    {
        return ConvertContainerToString(container, [](const auto& item) { return item; });
    }

    template <typename CharType>
    std::basic_string<CharType> StringOrEmptyIfNull(const CharType* string)
    {
        if (string)
        {
            return { string };
        }
        else
        {
            return {};
        }
    }

    // Converts the given bytes into a hexadecimal string.
    std::string ConvertToHexString(const std::vector<uint8_t>& buffer, size_t byteCount = 0);

    // Converts the given hexadecimal string into bytes.
    std::vector<uint8_t> ParseFromHexString(const std::string& value, size_t byteCount = 0);

    // Join a string vector using the provided separator.
    LocIndString Join(LocIndView separator, const std::vector<LocIndString>& vector);

    // Join a string vector using the provided separator.
    std::string Join(std::string_view separator, const std::vector<std::string>& vector);

    // Splits the string using the provided separator. Entries can also be trimmed.
    std::vector<std::string> Split(const std::string& input, char separator, bool trim = false);

    // Format an input string by replacing placeholders {index} with provided values at corresponding indices.
    // Note: After upgrading to C++20, this function should be deprecated in favor of std::format.
    template <typename ... T>
    std::string Format(std::string inputStr, T ... args)
    {
        int index = 0;
        (FindAndReplace(inputStr, "{" + std::to_string(index++) + "}", (std::ostringstream() << args).str()),...);
        return inputStr;
    }

    // Converts the given boolean value to a string.
    std::string_view ConvertBoolToString(bool value);

    // Converts the given GUID value to a string.
    std::string ConvertGuidToString(const GUID& value);

    // Creates a new GUID and returns the string value.
    std::wstring CreateNewGuidNameWString();

    // Converts the input string to a DWORD value using std::stoul and returns a boolean value based on the resulting DWORD value.
    bool IsDwordFlagSet(const std::string& value);

    // Finds the next control code index that would be replaced.
    // Returns std::string::npos if not found.
    size_t FindControlCodeToConvert(std::string_view input, size_t offset = 0);

    // Converts most control codes in the input to their corresponding control picture in the output.
    // Exempts tab, line feed, and carriage return from being replaced.
    std::string ConvertControlCodesToPictures(std::string_view input);
}
