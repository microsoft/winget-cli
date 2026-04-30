// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <winget/UserSettings.h>

#include <algorithm>
#include <numeric>
#include <vector>

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
        Utility::Version ParsedAvailableVersion;
        bool HasAvailableVersion = false;

        SortablePackageEntry() = default;

        SortablePackageEntry(
            size_t originalIndex,
            std::string_view name,
            std::string_view id,
            std::string_view installedVersion,
            std::string_view availableVersion,
            std::string_view source);
    };

    // Compares two sortable entries by the given field using precomputed sort keys.
    // Returns negative if a < b, positive if a > b, 0 if equal.
    int CompareByField(const SortablePackageEntry& a, const SortablePackageEntry& b, Settings::SortField field);

    // Sorts a vector of sortable entries by the given fields and direction.
    void SortEntries(
        std::vector<SortablePackageEntry>& entries,
        const std::vector<Settings::SortField>& sortFields,
        Settings::SortDirection direction);

    // Sorts a vector of arbitrary items by projecting each into a SortablePackageEntry
    // via a caller-supplied converter, sorting the projections, then reordering the
    // source vector to match. The converter signature is:
    //   SortablePackageEntry converter(const T& item, size_t index)
    template <typename T, typename Converter>
    void SortBy(
        std::vector<T>& items,
        Converter&& converter,
        const std::vector<Settings::SortField>& sortFields,
        Settings::SortDirection direction)
    {
        if (items.size() <= 1 || sortFields.empty())
        {
            return;
        }

        if (sortFields.size() == 1 && sortFields[0] == Settings::SortField::Relevance)
        {
            return;
        }

        std::vector<SortablePackageEntry> entries;
        entries.reserve(items.size());
        for (size_t i = 0; i < items.size(); ++i)
        {
            entries.push_back(converter(items[i], i));
        }

        SortEntries(entries, sortFields, direction);

        std::vector<T> sorted;
        sorted.reserve(items.size());
        for (const auto& entry : entries)
        {
            sorted.push_back(std::move(items[entry.OriginalIndex]));
        }
        items = std::move(sorted);
    }
}
