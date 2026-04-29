// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ListSortHelper.h"
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>

namespace AppInstaller::CLI::Workflow
{
    int CompareByField(const SortablePackageEntry& a, const SortablePackageEntry& b, Settings::SortField field)
    {
        using Settings::SortField;

        switch (field)
        {
        case SortField::Name:
            return Utility::FoldCase(std::string_view{ a.Name.get() }).compare(Utility::FoldCase(std::string_view{ b.Name.get() }));
        case SortField::Id:
            return Utility::FoldCase(std::string_view{ a.Id.get() }).compare(Utility::FoldCase(std::string_view{ b.Id.get() }));
        case SortField::Version:
        {
            Utility::Version va(a.InstalledVersion.get());
            Utility::Version vb(b.InstalledVersion.get());
            if (va < vb) return -1;
            if (vb < va) return 1;
            return 0;
        }
        case SortField::Source:
            return Utility::FoldCase(std::string_view{ a.Source.get() }).compare(Utility::FoldCase(std::string_view{ b.Source.get() }));
        case SortField::Available:
        {
            bool aHas = !a.AvailableVersion.get().empty();
            bool bHas = !b.AvailableVersion.get().empty();
            if (aHas != bHas)
            {
                // Ascending: has-update sorts before no-update
                return aHas ? -1 : 1;
            }
            if (aHas && bHas)
            {
                Utility::Version va(a.AvailableVersion.get());
                Utility::Version vb(b.AvailableVersion.get());
                if (va < vb) return -1;
                if (vb < va) return 1;
            }
            return 0;
        }
        default:
            return 0;
        }
    }

    // Currently used in tests; production code uses CompareByField with an index-based
    // permutation sort because workflow-specific row types carry additional fields.
    // Intended for reuse by commands (e.g., search, upgrade) whose data maps directly
    // to SortablePackageEntry without extra fields.
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
