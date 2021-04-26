// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Regex.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerLogging.h"

#define WINGET_THROW_REGEX_ERROR_IF_FAILED(_err_,_func_) \
    if (U_FAILURE(_err_)) \
    { \
        AICLI_LOG(Core, Error, << #_func_ " returned " << _err_); \
        THROW_HR(APPINSTALLER_CLI_ERROR_ICU_REGEX_ERROR); \
    }


namespace AppInstaller::Regex
{
    struct Expression::impl
    {
        using uregex_ptr = wil::unique_any<URegularExpression*, decltype(uregex_close), uregex_close>;
        using utext_ptr = wil::unique_any<UText*, decltype(utext_close), utext_close>;

        impl(std::string_view pattern, Options options)
        {
            UErrorCode uec = U_ZERO_ERROR;

            utext_ptr patternUtext{ utext_openUTF8(nullptr, pattern.data(), pattern.length(), &uec) };
            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, utext_openUTF8);

            // For now, just handle the one option
            uint32_t flags = 0;

            if (options == Options::CaseInsensitive)
            {
                flags = UREGEX_CASE_INSENSITIVE;
            }

            UParseError parseError{};

            m_regex.reset(uregex_openUText(patternUtext.get(), flags, &parseError, &uec));

            if (U_FAILURE(uec))
            {
                AICLI_LOG(Core, Error, << "uregex_openUText failed with error [" << uec << "] at line " << parseError.line << ", position " << parseError.offset << '\n' << pattern);
                THROW_HR(APPINSTALLER_CLI_ERROR_ICU_REGEX_ERROR);
            }
        }

        impl(const impl& other)
        {
            UErrorCode uec = U_ZERO_ERROR;

            m_regex.reset(uregex_clone(other.m_regex.get(), &uec));
            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_clone);
        }

        impl& operator=(const impl& other)
        {
            *this = impl{ other };
            return *this;
        }

        impl(impl&&) = default;
        impl& operator=(impl&&) = default;

        ~impl() = default;

        bool IsMatch(std::wstring_view input) const
        {
            UErrorCode uec = U_ZERO_ERROR;

            SetText(input);

            UBool result = uregex_matches(m_regex.get(), -1, &uec);
            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_matches);

            return !!result;
        }

        std::wstring Replace(std::wstring_view input, std::wstring_view replacement) const
        {
            UErrorCode uec = U_ZERO_ERROR;

            SetText(input);

            std::u16string_view u16replacement = Convert(replacement);
            utext_ptr replacementUtext{ utext_openUChars(nullptr, u16replacement.data(), u16replacement.length(), &uec) };
            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, utext_openUTF8);

            utext_ptr resultUText{ uregex_replaceAllUText(m_regex.get(), replacementUtext.get(), nullptr, &uec) };
            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_replaceAllUText);

            int64_t cch = utext_nativeLength(resultUText.get());
            std::wstring result(static_cast<size_t>(cch), '\0');

            utext_extract(resultUText.get(), 0, std::numeric_limits<int64_t>::max(), reinterpret_cast<char16_t*>(&result[0]), static_cast<int32_t>(result.size()), &uec);
            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, utext_extract);

            return result;
        }

        void ForEach(std::wstring_view input, const std::function<bool(bool, std::wstring_view)>&f) const
        {
            UErrorCode uec = U_ZERO_ERROR;

            SetText(input);
            int32_t startPos = 0;

            while (uregex_findNext(m_regex.get(), &uec))
            {
                WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_findNext);

                int32_t pos = uregex_start(m_regex.get(), 0, &uec);
                WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_start);
                THROW_HR_IF(E_UNEXPECTED, pos == -1);

                // First, send off the unmatched part before the match
                if (pos > startPos)
                {
                    if (!f(false, input.substr(startPos, static_cast<size_t>(pos) - startPos)))
                    {
                        return;
                    }
                }

                // Now send the matched part
                int32_t end = uregex_end(m_regex.get(), 0, &uec);
                WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_end);
                THROW_HR_IF(E_UNEXPECTED, end == -1);

                if (!f(true, input.substr(pos, static_cast<size_t>(end) - pos)))
                {
                    return;
                }

                startPos = end;
            }

            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_findNext);

            // Finally, send any remaining part
            if (input.length() > static_cast<size_t>(startPos))
            {
                f(false, input.substr(startPos));
            }
        }

    private:
        static std::u16string_view Convert(std::wstring_view input)
        {
            static_assert(sizeof(wchar_t) == sizeof(char16_t), "wchar_t and char16_t must be the same size");
            return { reinterpret_cast<const char16_t*>(input.data()), input.size() };
        }

        void SetText(std::wstring_view input) const
        {
            UErrorCode uec = U_ZERO_ERROR;

            std::u16string_view u16 = Convert(input);

            uregex_setText(m_regex.get(), u16.data(), static_cast<int32_t>(u16.length()), &uec);
            WINGET_THROW_REGEX_ERROR_IF_FAILED(uec, uregex_setText);
        }

        uregex_ptr m_regex;
    };

    Expression::Expression() = default;

    Expression::Expression(std::string_view pattern, Options options) : pImpl(std::make_unique<impl>(pattern, options)) {}

    Expression::Expression(const Expression& other)
    {
        if (other.pImpl)
        {
            pImpl = std::make_unique<impl>(*other.pImpl);
        }
    }

    Expression& Expression::operator=(const Expression& other)
    {
        return *this = Expression{ other };
    }

    Expression::Expression(Expression&&) noexcept = default;
    Expression& Expression::operator=(Expression&&) noexcept = default;

    Expression::~Expression() = default;

    Expression::operator bool() const
    {
        return static_cast<bool>(pImpl);
    }

    bool Expression::IsMatch(std::wstring_view input) const
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !pImpl);
        return pImpl->IsMatch(input);
    }

    std::wstring Expression::Replace(std::wstring_view input, std::wstring_view replacement) const
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !pImpl);
        return pImpl->Replace(input, replacement);
    }

    void Expression::ForEach(std::wstring_view input, const std::function<bool(bool, std::wstring_view)>& f) const
    {
        THROW_HR_IF(E_NOT_VALID_STATE, !pImpl);
        return pImpl->ForEach(input, f);
    }
}
