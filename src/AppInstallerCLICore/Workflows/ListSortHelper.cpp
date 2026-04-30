// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ListSortHelper.h"

namespace AppInstaller::CLI::Workflow
{
    SortablePackageEntry::SortablePackageEntry(
        size_t originalIndex,
        std::string_view name,
        std::string_view id,
        std::string_view installedVersion,
        std::string_view availableVersion,
        std::string_view source)
        : OriginalIndex(originalIndex),
          FoldedName(Utility::FoldCase(name)),
          FoldedId(Utility::FoldCase(id)),
          FoldedSource(Utility::FoldCase(source)),
          ParsedInstalledVersion(std::string{ installedVersion }),
          HasAvailableVersion(!availableVersion.empty())
    {
        if (HasAvailableVersion)
        {
            ParsedAvailableVersion = Utility::Version{ std::string{ availableVersion } };
        }
    }

    int CompareByField(const SortablePackageEntry& a, const SortablePackageEntry& b, Settings::SortField field)
    {
        using Settings::SortField;

        switch (field)
        {
        case SortField::Name:
            return a.FoldedName.compare(b.FoldedName);
        case SortField::Id:
            return a.FoldedId.compare(b.FoldedId);
        case SortField::Version:
        {
            if (a.ParsedInstalledVersion < b.ParsedInstalledVersion) return -1;
            if (b.ParsedInstalledVersion < a.ParsedInstalledVersion) return 1;
            return 0;
        }
        case SortField::Source:
            return a.FoldedSource.compare(b.FoldedSource);
        case SortField::Available:
        {
            if (a.HasAvailableVersion != b.HasAvailableVersion)
            {
                // Ascending: has-update sorts before no-update
                return a.HasAvailableVersion ? -1 : 1;
            }
            if (a.HasAvailableVersion && b.HasAvailableVersion)
            {
                if (a.ParsedAvailableVersion < b.ParsedAvailableVersion) return -1;
                if (b.ParsedAvailableVersion < a.ParsedAvailableVersion) return 1;
            }
            return 0;
        }
        default:
            return 0;
        }
    }

    void SortEntries(
        std::vector<SortablePackageEntry>& entries,
        const std::vector<Settings::SortField>& sortFields,
        Settings::SortDirection direction)
    {
        if (entries.size() <= 1 || sortFields.empty())
        {
            return;
        }

        // Relevance-only means no sorting
        if (sortFields.size() == 1 && sortFields[0] == Settings::SortField::Relevance)
        {
            return;
        }

        std::stable_sort(entries.begin(), entries.end(),
            [&sortFields, direction](const SortablePackageEntry& a, const SortablePackageEntry& b)
            {
                for (const auto& field : sortFields)
                {
                    int cmp = CompareByField(a, b, field);
                    if (cmp != 0)
                    {
                        return direction == Settings::SortDirection::Ascending ? (cmp < 0) : (cmp > 0);
                    }
                }
                return false;
            });
    }
}
