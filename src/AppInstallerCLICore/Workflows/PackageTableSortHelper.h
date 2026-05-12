// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <winget/UserSettings.h>

#include <algorithm>
#include <optional>
#include <vector>

namespace AppInstaller::CLI::Execution { struct Context; }

namespace AppInstaller::CLI::Workflow
{
    // Lightweight sortable representation of a package row with precomputed sort keys.
    // Decoupled from ICompositePackage/IPackageVersion to ease unit testing.
    struct SortablePackageEntry
    {
        size_t OriginalIndex = 0;

        // Precomputed case-folded sort keys
        std::string FoldedName;
        std::string FoldedId;
        std::string FoldedSource;

        // Precomputed parsed versions
        Utility::Version ParsedInstalledVersion;
        std::optional<Utility::Version> ParsedAvailableVersion;

        SortablePackageEntry() = default;

        SortablePackageEntry(
            size_t originalIndex,
            std::string_view name,
            std::string_view id,
            std::string_view installedVersion,
            std::string_view availableVersion,
            std::string_view source,
            Settings::SortField fieldMask);
    };

    // Compares two sortable entries by the given field using precomputed sort keys.
    // Returns negative if a < b, positive if a > b, 0 if equal.
    int CompareByField(const SortablePackageEntry& a, const SortablePackageEntry& b, Settings::SortField field);

    // Computes a bitmask of all sort fields so the constructor can skip unused fields.
    Settings::SortField ComputeSortFieldMask(const std::vector<Settings::SortField>& sortFields);

    // Result of sort parameter resolution. If ShouldSort is false, the caller should
    // preserve the current ordering (relevance or no-op).
    struct SortParameters
    {
        bool ShouldSort = false;
        std::vector<Settings::SortField> Fields;
        Settings::SortDirection Direction = Settings::SortDirection::Ascending;

        // Default constructor leaves ShouldSort=false. Used by unit tests to exercise
        // sort algorithm logic independently from CLI/settings parameter resolution.
        SortParameters() = default;

        // Resolves the effective sort parameters by reading CLI args, user settings,
        // and query context directly from the execution context.
        // Resolution order: explicit --sort args > settings > query-aware default.
        explicit SortParameters(const Execution::Context& context);
    };

    // Sorts a vector of sortable entries using the resolved sort parameters.
    void SortEntries(
        std::vector<SortablePackageEntry>& entries,
        const SortParameters& sortParams);

    // Sorts a vector of arbitrary items by projecting each into a SortablePackageEntry
    // via a caller-supplied converter, sorting the projections, then reordering the
    // source vector to match. The converter signature is:
    //   SortablePackageEntry converter(const T& item, size_t index)
    // The caller is responsible for pre-computing the SortFieldMask and capturing
    // it in the converter closure so the SortablePackageEntry constructor only
    // initializes the fields actually needed for comparison.
    template <typename T, typename Converter>
    void SortBy(
        std::vector<T>& items,
        Converter&& converter,
        const SortParameters& sortParams)
    {
        if (items.size() <= 1 || !sortParams.ShouldSort)
        {
            return;
        }

        std::vector<SortablePackageEntry> entries;
        entries.reserve(items.size());
        for (size_t i = 0; i < items.size(); ++i)
        {
            entries.push_back(converter(items[i], i));
        }

        SortEntries(entries, sortParams);

        std::vector<T> sorted;
        sorted.reserve(items.size());
        for (const auto& entry : entries)
        {
            sorted.push_back(std::move(items[entry.OriginalIndex]));
        }
        items = std::move(sorted);
    }
}
