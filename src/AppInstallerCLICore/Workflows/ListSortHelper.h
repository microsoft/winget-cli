// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/LocIndependent.h>
#include <winget/UserSettings.h>

#include <vector>

namespace AppInstaller::CLI::Workflow
{
    // Lightweight sortable representation of a package line in list output.
    // Decoupled from ICompositePackage/IPackageVersion to ease unit testing.
    struct SortablePackageEntry
    {
        Utility::LocIndString Name;
        Utility::LocIndString Id;
        Utility::LocIndString InstalledVersion;
        Utility::LocIndString AvailableVersion;
        Utility::LocIndString Source;
    };

    // Compares two sortable entries by the given field.
    // Returns negative if a < b, positive if a > b, 0 if equal.
    int CompareByField(const SortablePackageEntry& a, const SortablePackageEntry& b, Settings::SortField field);

    // Sorts a vector of sortable entries by the given fields and direction.
    // Currently used in tests; production code uses CompareByField with an index-based
    // permutation sort because workflow-specific row types carry additional fields.
    // Intended for reuse by commands (e.g., search, upgrade) whose data maps directly
    // to SortablePackageEntry without extra fields.
    void SortEntries(
        std::vector<SortablePackageEntry>& entries,
        const std::vector<Settings::SortField>& sortFields,
        Settings::SortDirection direction);
}
