// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"
#include "Invocation.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::CLI
{
    // Data created by CompleteCommand to be consumed by other commands in order
    // to provide context sensitive results.
    struct CompletionData
    {
        CompletionData(std::string_view word, std::string_view commandLine, std::string_view position);

        const std::string& Word() const { return m_word; }
        Invocation& BeforeWord() const { return *m_argsBeforeWord; }
        Invocation& AfterWord() const { return *m_argsAfterWord; }

    private:
        static void ParseInto(std::string_view line, std::vector<std::string>& args, bool skipFirst);

        std::string m_word;
        std::unique_ptr<CLI::Invocation> m_argsBeforeWord;
        std::unique_ptr<CLI::Invocation> m_argsAfterWord;
    };
}
