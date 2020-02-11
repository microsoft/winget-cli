// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>


namespace AppInstaller::Repository
{
    // The type of matching to perform during a search.
    enum class MatchType
    {
        Exact,
        Substring,
        Wildcard,
        Fuzzy,
    };

    // The field to match on.
    enum class MatchField
    {
        Id,
        Name,
        Moniker,
        Version,
        Channel,
        Tag,
        Command,
        Protocol,
        Extension,
    };

    // A single match to be performed during a search.
    struct RequestMatch
    {
        MatchType Type;
        std::string Value;

        RequestMatch(MatchType t, std::string v) : Type(t), Value(std::move(v)) {}
    };

    // A match on a specific field to be performed during a search.
    struct MatchFilter : public RequestMatch
    {
        MatchField Field;

        MatchFilter(MatchField f, MatchType t, std::string v) : RequestMatch(t, std::move(v)), Field(f) {}
    };

    // Container for data used to filter the available manifests in a source.
    struct SearchRequest
    {
        // The generic query matches against a source defined set of fields.
        // If not provided, the filters should be used against the entire dataset.
        std::optional<RequestMatch> Query;

        // Specific fields used to filter the data further.
        std::vector<MatchFilter> Filters;

        // The maximum number of results to return.
        // The default of 0 will place no limit.
        size_t MaximumResults{};
    };

    // A single application result from a search.
    struct IApplication
    {
        virtual std::vector<std::unique_ptr<IManifest>> GetMatchingManifests() const = 0;
    };

    // A single result from the search.
    struct ResultMatch
    {
        // The application found by the search request.
        std::unique_ptr<IApplication> Application;

        // The highest order field on which the application matched the search.
        MatchFilter Field;

        ResultMatch(std::unique_ptr<IApplication> a, MatchFilter f) : Application(std::move(a)), Field(std::move(f)) {}
    };

    // Search result data.
    struct SearchResult
    {
        // The full set of results from the search.
        std::vector<ResultMatch> Matches;
    };
}
