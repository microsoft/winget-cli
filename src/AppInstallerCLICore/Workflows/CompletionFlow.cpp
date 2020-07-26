// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "CompletionFlow.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::Utility::literals;

    void CompleteSourceName(Execution::Context& context)
    {
        const std::string& word = context.Get<Data::CompletionData>().Word();

        for (const auto& source : Repository::GetSources())
        {
            if (word.empty() || Utility::CaseInsensitiveStartsWith(source.Name, word))
            {
                context.Reporter.Info() << source.Name << std::endl;
            }
        }
    }
}
