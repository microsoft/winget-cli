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
                if (match1.MatchCriteria.Type != match2.MatchCriteria.Type)
                {
                    return match2.MatchCriteria.Type > match1.MatchCriteria.Type;
                }

                if (match1.MatchCriteria.Field != match2.MatchCriteria.Field)
                {
                    return match2.MatchCriteria.Field > match1.MatchCriteria.Field;
                }

                return false;
            }
        };

        std::stable_sort(matches.begin(), matches.end(), ResultMatchComparator());
    }
}

