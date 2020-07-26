// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <ostream>
#include <string>
#include <string_view>

namespace AppInstaller::Utility
{
    // Compares the two UTF8 strings in a case insensitive manner.
    bool CaseInsensitiveEquals(std::string_view a, std::string_view b);

    // Determins if string a starts with string b.
    bool CaseInsensitiveStartsWith(std::string_view a, std::string_view b);

    // Converts the given UTF16 string to UTF8
    std::string ConvertToUTF8(std::wstring_view input);

    // Converts the given UTF8 string to UTF16
    std::wstring ConvertToUTF16(std::string_view input);

    // Returns the number of grapheme clusters (characters) in an UTF8-encoded string.
    size_t UTF8Length(std::string_view input);

    // Returns a substring view in an UTF8-encoded string. Offset and count are measured in grapheme clusters (characters).
    std::string_view UTF8Substring(std::string_view input, size_t offset, size_t count);

    // Normalizes a UTF8 string to the given form.
    std::string Normalize(std::string_view input, NORM_FORM form = NORM_FORM::NormalizationKC);

    // Normalizes a UTF16 string to the given form.
    std::wstring Normalize(std::wstring_view input, NORM_FORM form = NORM_FORM::NormalizationKC);

    // Type to hold and force a normalized UTF8 string.
    template <NORM_FORM Form = NORM_FORM::NormalizationKC>
    struct NormalizedUTF8 : public std::string
    {
        NormalizedUTF8() = default;

        template <size_t Size>
        NormalizedUTF8(const char (&s)[Size]) : std::string(Normalize(std::string_view{ s, (s[Size - 1] == '\0' ? Size - 1 : Size) }, Form)) {}

        NormalizedUTF8(std::string_view sv) : std::string(Normalize(sv, Form)) {}

        NormalizedUTF8(const std::string& s) : std::string(Normalize(s, Form)) {}
        NormalizedUTF8(std::string&& s) : std::string(Normalize(s, Form)) {}

        NormalizedUTF8(std::wstring_view sv) : std::string(ConvertToUTF8(Normalize(sv, Form))) {}

        NormalizedUTF8(const NormalizedUTF8& other) = default;
        NormalizedUTF8& operator=(const NormalizedUTF8& other) = default;

        NormalizedUTF8(NormalizedUTF8&& other) = default;
        NormalizedUTF8& operator=(NormalizedUTF8&& other) = default;

        template <size_t Size>
        NormalizedUTF8& operator=(const char(&s)[Size])
        {
            assign(Normalize(std::string_view{ s, (s[Size - 1] == '\0' ? Size - 1 : Size) }, Form));
            return *this;
        }

        NormalizedUTF8& operator=(std::string_view sv)
        {
            assign(Normalize(sv, Form));
            return *this;
        }

        NormalizedUTF8& operator=(const std::string& s)
        {
            assign(Normalize(s, Form));
            return *this;
        }

        NormalizedUTF8& operator=(std::string&& s)
        {
            assign(Normalize(s, Form));
            return *this;
        }
    };

    using NormalizedString = NormalizedUTF8<>;

    // Get the lower case version of the given std::string
    std::string ToLower(std::string_view in);

    // Get the lower case version of the given std::wstring
    std::wstring ToLower(std::wstring_view in);

    // Checks if the input string is empty or whitespace
    bool IsEmptyOrWhitespace(std::wstring_view str);

    // Find token in the input string and replace with value.
    // Returns a value indicating whether a replacement occurred.
    bool FindAndReplace(std::string& inputStr, std::string_view token, std::string_view value);

    // Removes whitespace from the beginning and end of the string.
    std::string& Trim(std::string& str);

    // Reads the entire stream into a string.
    std::string ReadEntireStream(std::istream& stream);
}
