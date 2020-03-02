// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Manifest/Manifest.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
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
    enum class ApplicationMatchField
    {
        Id,
        Name,
        Moniker,
        Tag,
        Command,
    };

    // A single match to be performed during a search.
    struct RequestMatch
    {
        MatchType Type;
        std::string Value;

        RequestMatch(MatchType t, std::string v) : Type(t), Value(std::move(v)) {}
    };

    // A match on a specific field to be performed during a search.
    struct ApplicationMatchFilter : public RequestMatch
    {
        ApplicationMatchField Field;

        ApplicationMatchFilter(ApplicationMatchField f, MatchType t, std::string v) : RequestMatch(t, std::move(v)), Field(f) {}
    };

    // Container for data used to filter the available manifests in a source.
    struct SearchRequest
    {
        // The generic query matches against a source defined set of fields.
        // If not provided, the filters should be used against the entire dataset.
        std::optional<RequestMatch> Query;

        // Specific fields used to filter the data further.
        std::vector<ApplicationMatchFilter> Filters;

        // The maximum number of results to return.
        // The default of 0 will place no limit.
        size_t MaximumResults{};
    };

    // A single application result from a search.
    struct IApplication
    {
        // Gets the id of the application.
        virtual std::string GetId() = 0;

        // Gets the name of the application (the latest name).
        virtual std::string GetName() = 0;

        // Gets a manifest for this application.
        // An empty version implies 'latest'.
        // An empty channel is the 'general audience'.
        virtual Manifest::Manifest GetManifest(std::string_view version, std::string_view channel) = 0;

        // Gets all versions of this application.
        // The pair is <version, channel>.
        // The versions will be returned in sorted, desceding order.
        //  Ex. { 4, 3, 2, 1 }
        virtual std::vector<std::pair<std::string, std::string>> GetVersions() = 0;
    };

    // A single result from the search.
    struct ResultMatch
    {
        // The application found by the search request.
        std::unique_ptr<IApplication> Application;

        // The highest order field on which the application matched the search.
        ApplicationMatchFilter MatchCriteria;

        ResultMatch(std::unique_ptr<IApplication>&& a, ApplicationMatchFilter f) : Application(std::move(a)), MatchCriteria(std::move(f)) {}
    };

    // Search result data.
    struct SearchResult
    {
        // The full set of results from the search.
        std::vector<ResultMatch> Matches;
    };

    inline std::string_view ApplicationMatchFieldToString(ApplicationMatchField matchField)
    {
        using namespace std::string_view_literals;

        switch (matchField)
        {
        case ApplicationMatchField::Command:
            return "Command"sv;
        case ApplicationMatchField::Id:
            return "Id"sv;
        case ApplicationMatchField::Moniker:
            return "Moniker"sv;
        case ApplicationMatchField::Name:
            return "Name"sv;
        case ApplicationMatchField::Tag:
            return "Tag"sv;
        }

        return "UnknownMatchField"sv;
    }
}
