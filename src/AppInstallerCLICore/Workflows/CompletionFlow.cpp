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
                OutputCompletionString(stream, searchResult.Matches[i].Package->GetProperty(Repository::PackageProperty::Id));
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

        for (const auto& vc : context.Get<Execution::Data::SearchResult>().Matches[0].Package->GetAvailableVersionKeys())
        {
            if (word.empty() || Utility::CaseInsensitiveStartsWith(vc.Version, word))
            {
                OutputCompletionString(stream, vc.Version);
            }
        }
    }

    void CompleteWithSearchResultChannels(Execution::Context& context)
    {
        const std::string& word = context.Get<Data::CompletionData>().Word();
        auto stream = context.Reporter.Completion();

        std::vector<std::string> channels;

        for (const auto& vc : context.Get<Execution::Data::SearchResult>().Matches[0].Package->GetAvailableVersionKeys())
        {
            if ((word.empty() || Utility::CaseInsensitiveStartsWith(vc.Channel, word)) &&
                std::find(channels.begin(), channels.end(), vc.Channel) == channels.end())
            {
                channels.emplace_back(vc.Channel);
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
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Tag:
        case Execution::Args::Type::Command:
        case Execution::Args::Type::Version:
        case Execution::Args::Type::Channel:
            context <<
                Workflow::OpenSource;
            break;
        }

        context << CompleteWithSingleSemanticsForValueUsingExistingSource(m_type);
    }

    void CompleteWithSingleSemanticsForValueUsingExistingSource::operator()(Execution::Context& context) const
    {
        switch (m_type)
        {
        case Execution::Args::Type::Query:
            context <<
                Workflow::RequireCompletionWordNonEmpty <<
                Workflow::SearchSourceForSingleCompletion <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Manifest:
            // Intentionally output none to enable pass through to filesystem.
            break;
        case Execution::Args::Type::Id:
            context <<
                Workflow::SearchSourceForCompletionField(Repository::PackageMatchField::Id) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Name:
            context <<
                Workflow::SearchSourceForCompletionField(Repository::PackageMatchField::Name) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Moniker:
            context <<
                Workflow::SearchSourceForCompletionField(Repository::PackageMatchField::Moniker) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Tag:
            context <<
                Workflow::SearchSourceForCompletionField(Repository::PackageMatchField::Tag) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Command:
            context <<
                Workflow::SearchSourceForCompletionField(Repository::PackageMatchField::Command) <<
                Workflow::CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Version:
            // Here we require that the standard search finds a single entry, and we list those versions.
            context <<
                Workflow::SearchSourceForSingle <<
                Workflow::EnsureOneMatchFromSearchResult <<
                Workflow::CompleteWithSearchResultVersions;
            break;
        case Execution::Args::Type::Channel:
            // Here we require that the standard search finds a single entry, and we list those channels.
            context <<
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
