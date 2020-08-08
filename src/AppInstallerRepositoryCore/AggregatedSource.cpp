// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AggregatedSource.h"

namespace AppInstaller::Repository
{
    AggregatedSource::AggregatedSource()
    {
        m_details.Name = "AggregatedSource";
        m_details.IsAggregated = true;
    }

    const SourceDetails& AppInstaller::Repository::AggregatedSource::GetDetails() const
    {
        return m_details;
    }

    void AggregatedSource::AddSource(std::shared_ptr<ISource> source)
    {
        m_sources.emplace_back(std::move(source));
    }

    SearchResult AggregatedSource::Search(const SearchRequest& request)
    {
        SearchResult result;

        for (auto& source : m_sources)
        {
            auto oneSourceResult = source->Search(request);

            for (auto& r : oneSourceResult.Matches)
            {
                r.SourceName = source->GetDetails().Name;
                result.Matches.emplace_back(std::move(r));
            }
        }

        SortResultMatches(result.Matches);

        if (request.MaximumResults > 0 && result.Matches.size() > request.MaximumResults)
        {
            result.Truncated = true;
            result.Matches.erase(result.Matches.begin() + request.MaximumResults, result.Matches.end());
        }

        return result;
    }

    void AggregatedSource::SortResultMatches(std::vector<ResultMatch>& matches)
    {
        struct ResultMatchComparator
        {
            // The comparator compares the ResultMatch by MatchType first, then Field in a predefined order.
            bool operator() (
                const ResultMatch& match1,
                const ResultMatch& match2)
            {
                std::vector<MatchType> MatchTypeOrder =
                { MatchType::Exact, MatchType::CaseInsensitive, MatchType::StartsWith, MatchType::Fuzzy,
                  MatchType::Substring, MatchType::FuzzySubstring, MatchType::Wildcard };

                auto matchTypeOrder1 = std::find(MatchTypeOrder.begin(), MatchTypeOrder.end(), match1.MatchCriteria.Type);
                auto matchTypeOrder2 = std::find(MatchTypeOrder.begin(), MatchTypeOrder.end(), match2.MatchCriteria.Type);
                auto matchTypeDistance = std::distance(matchTypeOrder1, matchTypeOrder2);

                if (matchTypeDistance != 0)
                {
                    return matchTypeDistance > 0;
                }

                std::vector<ApplicationMatchField> MatchFieldOrder =
                { ApplicationMatchField::Id, ApplicationMatchField::Name, ApplicationMatchField::Moniker,
                  ApplicationMatchField::Command, ApplicationMatchField::Tag };

                auto matchFieldOrder1 = std::find(MatchFieldOrder.begin(), MatchFieldOrder.end(), match1.MatchCriteria.Field);
                auto matchFieldOrder2 = std::find(MatchFieldOrder.begin(), MatchFieldOrder.end(), match2.MatchCriteria.Field);
                auto matchFieldDistance = std::distance(matchFieldOrder1, matchFieldOrder2);

                if (matchFieldDistance != 0)
                {
                    return matchFieldDistance > 0;
                }

                return false;
            }
        };

        std::stable_sort(matches.begin(), matches.end(), ResultMatchComparator());
    }
}

