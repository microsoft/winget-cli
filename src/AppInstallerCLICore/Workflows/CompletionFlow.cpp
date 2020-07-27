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
                context.Reporter.Completion() << source.Name << std::endl;
            }
        }
    }

    void RequireCompletionWordNonEmpty(Execution::Context& context)
    {
        if (context.Get<Data::CompletionData>().Word().empty())
        {
            AICLI_LOG(CLI, Verbose, << "Completion word empty, cannot complete");
            AICLI_TERMINATE_CONTEXT(E_NOT_SET);
        }
    }

    void CompleteWithSearchResultField::operator()(Execution::Context& context) const
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();
        auto stream = context.Reporter.Completion();

        using FieldFunc = Utility::LocIndString (Repository::IApplication::*)();
        FieldFunc fieldFunc = nullptr;
        switch (m_field)
        {
        case Repository::ApplicationMatchField::Id:
            fieldFunc = &Repository::IApplication::GetId;
            break;
        case Repository::ApplicationMatchField::Name:
            fieldFunc = &Repository::IApplication::GetName;
            break;
        //case Repository::ApplicationMatchField::Moniker:
        //    fieldFunc = &Repository::IApplication::get;
        //    break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            auto app = searchResult.Matches[i].Application.get();
            stream << (app->*fieldFunc)() << std::endl;
        }
    }
}
