// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "CompletionFlow.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::Utility::literals;

    namespace
    {
        // Outputs the completion string, wrapping it in quotes if needed.
        void OutputCompletionString(Execution::NoVTStream& stream, std::string_view value)
        {
            if (value.find_first_of(' ') != std::string_view::npos)
            {
                stream << '"' << value << '"' << std::endl;
            }
            else
            {
                stream << value << std::endl;
            }
        }
    }

    void CompleteSourceName(Execution::Context& context)
    {
        const std::string& word = context.Get<Data::CompletionData>().Word();
        auto stream = context.Reporter.Completion();

        for (const auto& source : Repository::GetSources())
        {
            if (word.empty() || Utility::CaseInsensitiveStartsWith(source.Name, word))
            {
                OutputCompletionString(stream, source.Name);
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

    void CompleteWithMatchedField(Execution::Context& context)
    {
        auto& searchResult = context.Get<Execution::Data::SearchResult>();
        auto stream = context.Reporter.Completion();

        for (size_t i = 0; i < searchResult.Matches.size(); ++i)
        {
            if (searchResult.Matches[i].MatchCriteria.Value.empty())
            {
                OutputCompletionString(stream, searchResult.Matches[i].Application->GetId());
            }
            else
            {
                OutputCompletionString(stream, searchResult.Matches[i].MatchCriteria.Value);
            }
        }
    }

    void CompleteWithSearchResultVersions(Execution::Context& context)
    {
        const std::string& word = context.Get<Data::CompletionData>().Word();
        auto stream = context.Reporter.Completion();

        for (const auto& vc : context.Get<Execution::Data::SearchResult>().Matches[0].Application->GetVersions())
        {
            std::string version = vc.GetVersion().ToString();
            if (word.empty() || Utility::CaseInsensitiveStartsWith(version, word))
            {
                OutputCompletionString(stream, version);
            }
        }
    }

    void CompleteWithSearchResultChannels(Execution::Context& context)
    {
        const std::string& word = context.Get<Data::CompletionData>().Word();
        auto stream = context.Reporter.Completion();

        std::vector<std::string> channels;

        for (const auto& vc : context.Get<Execution::Data::SearchResult>().Matches[0].Application->GetVersions())
        {
            std::string channel = vc.GetChannel().ToString();
            if ((word.empty() || Utility::CaseInsensitiveStartsWith(channel, word)) &&
                std::find(channels.begin(), channels.end(), channel) == channels.end())
            {
                channels.emplace_back(std::move(channel));
            }
        }

        for (const auto& c : channels)
        {
            OutputCompletionString(stream, c);
        }
    }

    void CompleteWithSingleSemanticsForValue::operator()(Execution::Context& context) const
    {
        switch (m_type)
        {
        case Execution::Args::Type::Query:
            context <<
                Workflow::OpenSource <<
                Workflow::RequireCompletionWordNonEmpty <<
                Workflow::SearchSourceForSingleCompletion <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Manifest:
            // Intentionally output none to enable pass through to filesystem.
            break;
        case Execution::Args::Type::Id:
            context <<
                Workflow::OpenSource <<
                Workflow::SearchSourceForCompletionField(Repository::ApplicationMatchField::Id) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Name:
            context <<
                Workflow::OpenSource <<
                Workflow::SearchSourceForCompletionField(Repository::ApplicationMatchField::Name) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Moniker:
            context <<
                Workflow::OpenSource <<
                Workflow::SearchSourceForCompletionField(Repository::ApplicationMatchField::Moniker) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Tag:
            context <<
                Workflow::OpenSource <<
                Workflow::SearchSourceForCompletionField(Repository::ApplicationMatchField::Tag) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Command:
            context <<
                Workflow::OpenSource <<
                Workflow::SearchSourceForCompletionField(Repository::ApplicationMatchField::Command) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Version:
            // Here we require that the standard search finds a single entry, and we list those versions.
            context <<
                Workflow::OpenSource <<
                Workflow::SearchSourceForSingle <<
                Workflow::EnsureOneMatchFromSearchResult <<
                Workflow::CompleteWithSearchResultVersions;
            break;
        case Execution::Args::Type::Channel:
            // Here we require that the standard search finds a single entry, and we list those channels.
            context <<
                Workflow::OpenSource <<
                Workflow::SearchSourceForSingle <<
                Workflow::EnsureOneMatchFromSearchResult <<
                Workflow::CompleteWithSearchResultChannels;
            break;
        case Execution::Args::Type::Source:
            context <<
                Workflow::CompleteSourceName;
            break;
        }
    }

    void CompleteWithEmptySet(Execution::Context& context)
    {
        context.Reporter.Completion() << std::endl;
    }
}
