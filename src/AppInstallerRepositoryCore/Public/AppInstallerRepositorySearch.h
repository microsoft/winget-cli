// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Manifest/Manifest.h>
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <winget/LocIndependent.h>

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
        FuzzySubstring,
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
        Utility::NormalizedString Value;

        RequestMatch(MatchType t, std::string_view v) : Type(t), Value(v) {}
    };

    // A match on a specific field to be performed during a search.
    struct ApplicationMatchFilter : public RequestMatch
    {
        ApplicationMatchField Field;

        ApplicationMatchFilter(ApplicationMatchField f, MatchType t, std::string_view v) : RequestMatch(t, v), Field(f) {}
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

        // Returns a string summarizing the search request.
        std::string ToString() const;
    };

    // A single application result from a search.
    struct IApplication
    {
        virtual ~IApplication() = default;

        // Gets the id of the application.
        virtual Utility::LocIndString GetId() = 0;

        // Gets the name of the application (the latest name).
        virtual Utility::LocIndString GetName() = 0;

        // Gets a manifest for this application.
        // An empty version implies 'latest'.
        // An empty channel is the 'general audience'.
        virtual std::optional<Manifest::Manifest> GetManifest(const Utility::NormalizedString& version, const Utility::NormalizedString& channel) = 0;

        // Gets all versions of this application.
        // The versions will be returned in sorted, descending order.
        //  Ex. { 4, 3, 2, 1 }
        virtual std::vector<Utility::VersionAndChannel> GetVersions() = 0;
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

        // If true, the results were truncated by the given SearchRequest::MaximumResults.
        bool Truncated = false;
    };

    inline std::string_view MatchTypeToString(MatchType type)
    {
        using namespace std::string_view_literals;

        switch (type)
        {
        case MatchType::Exact:
            return "Exact"sv;
        case MatchType::Substring:
            return "Substring"sv;
        case MatchType::Wildcard:
            return "Wildcard"sv;
        case MatchType::Fuzzy:
            return "Fuzzy"sv;
        case MatchType::FuzzySubstring:
            return "FuzzySubstring"sv;
        }

        return "UnknownMatchType"sv;
    }

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
