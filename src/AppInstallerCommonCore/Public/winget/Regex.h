// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <functional>
#include <memory>
#include <string_view>


namespace AppInstaller::Regex
{
    // Options for regular expression use.
    enum class Options
    {
        None = 0,
        CaseInsensitive,
    };

    // Stores the compiled regular expression.
    // All pattern strings are considered UTF-8.
    // All input strings are considered UTF-16, as this is what ICU operates on internally.
    struct Expression
    {
        Expression();
        Expression(std::string_view pattern, Options options = Options::None);

        Expression(const Expression&);
        Expression& operator=(const Expression&);

        Expression(Expression&&);
        Expression& operator=(Expression&&);

        ~Expression();

        // Determines if the expression contains a value.
        operator bool() const;

        // Returns a value indicating whether the *entire* input matches the expression.
        bool IsMatch(std::wstring_view input) const;

        // Replaces all matches in the input with the replacement.
        std::wstring Replace(std::wstring_view input, std::wstring_view replacement) const;

        // For each section of the input, invoke the given functor. This allows the caller
        // to iterate over the entire string, taking action as appropriate for each part.
        // The parameters are:
        //      bool :: indicates whether this section was a match
        //      string_view :: the text for the section
        // The functor should return true to continue the loop, or false to break it.
        void ForEach(std::wstring_view input, const std::function<bool(bool,std::wstring_view)>& f) const;

    private:
        struct impl;
        std::unique_ptr<impl> pImpl;
    };
}
