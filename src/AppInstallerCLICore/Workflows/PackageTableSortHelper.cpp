// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageTableSortHelper.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace Settings;

    SortablePackageEntry::SortablePackageEntry(
        size_t originalIndex,
        std::string_view name,
        std::string_view id,
        std::string_view installedVersion,
        std::string_view availableVersion,
        std::string_view source,
        SortField fieldMask)
        : OriginalIndex(originalIndex)
    {
        if (WI_IsFlagSet(fieldMask, SortField::Name))
        {
            FoldedName = Utility::FoldCase(name);
        }
        if (WI_IsFlagSet(fieldMask, SortField::Id))
        {
            FoldedId = Utility::FoldCase(id);
        }
        if (WI_IsFlagSet(fieldMask, SortField::Source))
        {
            FoldedSource = Utility::FoldCase(source);
        }
        if (WI_IsFlagSet(fieldMask, SortField::Version))
        {
            ParsedInstalledVersion = Utility::Version{ std::string{ installedVersion } };
        }
        if (WI_IsFlagSet(fieldMask, SortField::Available))
        {
            if (!availableVersion.empty())
            {
                ParsedAvailableVersion = Utility::Version{ std::string{ availableVersion } };
            }
        }
    }

    SortField ComputeSortFieldMask(const std::vector<SortField>& sortFields)
    {
        SortField mask = SortField::None;
        for (const auto& f : sortFields)
        {
            mask |= f;
        }
        return mask;
    }

    SortParameters ResolveSortParameters(
        const std::vector<std::string_view>& explicitSortArgs,
        bool hasQuery,
        bool hasExplicitAscending,
        bool hasExplicitDescending)
    {
        SortParameters result;
        std::vector<SortField> sortFields;

        if (!explicitSortArgs.empty())
        {
            for (const auto& arg : explicitSortArgs)
            {
                auto field = ConvertToSortField(arg);
                WI_ASSERT(field.has_value());
                if (field.has_value())
                {
                    sortFields.emplace_back(field.value());
                }
            }
        }
        else
        {
            sortFields = User().Get<Setting::OutputSortOrder>();

            if (sortFields.empty())
            {
                if (hasQuery)
                {
                    // Preserve relevance ordering when a free-text query is present
                    // and no explicit sort preference is configured.
                    return result; // ShouldSort = false
                }

                // No settings, no query — default to name sort.
                sortFields.emplace_back(SortField::Name);
            }
        }

        // Relevance-only means preserve source ordering.
        if (sortFields.size() == 1 && sortFields[0] == SortField::Relevance)
        {
            return result; // ShouldSort = false
        }

        // Resolve direction
        SortDirection direction = SortDirection::Ascending;
        if (hasExplicitDescending)
        {
            direction = SortDirection::Descending;
        }
        else if (hasExplicitAscending)
        {
            direction = SortDirection::Ascending;
        }
        else
        {
            direction = User().Get<Setting::OutputSortDirection>();
        }

        result.ShouldSort = true;
        result.Fields = std::move(sortFields);
        result.Direction = direction;
        return result;
    }

    int CompareByField(const SortablePackageEntry& a, const SortablePackageEntry& b, SortField field)
    {
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
            // std::optional comparison: nullopt < any value.
            // We want has-version to sort before no-version in ascending order,
            // so reverse the comparison operands.
            if (a.ParsedAvailableVersion.has_value() != b.ParsedAvailableVersion.has_value())
            {
                return a.ParsedAvailableVersion.has_value() ? -1 : 1;
            }
            if (a.ParsedAvailableVersion.has_value() && b.ParsedAvailableVersion.has_value())
            {
                if (a.ParsedAvailableVersion.value() < b.ParsedAvailableVersion.value()) return -1;
                if (b.ParsedAvailableVersion.value() < a.ParsedAvailableVersion.value()) return 1;
            }
            return 0;
        }
        case SortField::Relevance:
            // Relevance has no precomputed sort key — preserve original ordering.
            return 0;
        default:
            WI_ASSERT(false);
            return 0;
        }
    }

    void SortEntries(
        std::vector<SortablePackageEntry>& entries,
        const std::vector<SortField>& sortFields,
        SortDirection direction)
    {
        if (entries.size() <= 1 || sortFields.empty())
        {
            return;
        }

        // Relevance-only means no sorting
        if (sortFields.size() == 1 && sortFields[0] == SortField::Relevance)
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
                        return direction == SortDirection::Ascending ? (cmp < 0) : (cmp > 0);
                    }
                }
                return false;
            });
    }
}
