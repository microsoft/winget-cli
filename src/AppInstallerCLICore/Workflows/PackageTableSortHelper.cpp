// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PackageTableSortHelper.h"
#include "ExecutionContext.h"

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

    SortParameters::SortParameters(const Execution::Context& context)
    {
        if (context.Args.Contains(Execution::Args::Type::Sort))
        {
            for (const auto& arg : *context.Args.GetArgs(Execution::Args::Type::Sort))
            {
                auto field = ConvertToSortField(arg);
                WI_ASSERT(field.has_value());
                if (field.has_value())
                {
                    Fields.emplace_back(field.value());
                }
            }
        }
        else
        {
            Fields = User().Get<Setting::OutputSortOrder>();

            if (Fields.empty())
            {
                bool hasQuery = context.Args.Contains(Execution::Args::Type::Query) ||
                    context.Args.Contains(Execution::Args::Type::MultiQuery);

                if (hasQuery)
                {
                    // Preserve relevance ordering when a free-text query is present
                    // and no explicit sort preference is configured.
                    return; // ShouldSort remains false
                }

                // No settings, no query — default to name sort.
                Fields.emplace_back(SortField::Name);
            }
        }

        // Relevance-only means preserve source ordering.
        if (Fields.size() == 1 && Fields[0] == SortField::Relevance)
        {
            return; // ShouldSort remains false
        }

        // Resolve direction: CLI flags override settings
        Direction = context.Args.Contains(Execution::Args::Type::SortDescending) ? SortDirection::Descending :
            context.Args.Contains(Execution::Args::Type::SortAscending) ? SortDirection::Ascending :
            User().Get<Setting::OutputSortDirection>();

        ShouldSort = true;
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
            bool aHas = a.ParsedAvailableVersion.has_value();
            bool bHas = b.ParsedAvailableVersion.has_value();

            // Has-version sorts before no-version in ascending order.
            if (aHas != bHas)
            {
                return aHas ? -1 : 1;
            }

            // Both have versions — compare normally.
            if (aHas && bHas)
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
        const SortParameters& sortParams)
    {
        if (entries.size() <= 1 || !sortParams.ShouldSort)
        {
            return;
        }

        std::stable_sort(entries.begin(), entries.end(),
            [&sortParams](const SortablePackageEntry& a, const SortablePackageEntry& b)
            {
                for (const auto& field : sortParams.Fields)
                {
                    int cmp = CompareByField(a, b, field);
                    if (cmp != 0)
                    {
                        return sortParams.Direction == SortDirection::Ascending ? (cmp < 0) : (cmp > 0);
                    }
                }
                return false;
            });
    }
}
