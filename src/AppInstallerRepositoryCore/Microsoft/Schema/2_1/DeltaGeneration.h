// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/2_0/PackageUpdateTrackingTable.h"
#include <winget/SQLiteWrapper.h>
#include <winget/SQLiteVersion.h>
#include <filesystem>
#include <set>
#include <string>
#include <vector>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    // Writes a delta database describing the difference between a baseline index and the index
    // that is currently being packaged.
    //
    // This must run while the index being packaged still holds its update tracking table, as that
    // is the only record of which packages have changed since the baseline was produced.
    //
    // The source and the baseline assign the same rowid to a given package, so a package that
    // exists in both is described by rows that carry its baseline rowid, and a package that is new
    // to the source carries a rowid that the baseline cannot have used.
    //
    // The version is recorded as the delta's own schema version, so that opening the delta selects
    // the interface that knows how to merge it with a baseline.
    //
    // Nothing appears at the output path until the delta is complete: it is built beside the
    // destination and moved into place only on success, so a failure partway through cannot leave
    // something that looks like a usable delta. An output path that already exists is refused.
    void Generate(
        const SQLite::Connection& sourceConnection,
        const SQLite::Connection& baselineConnection,
        const std::filesystem::path& deltaOutputPath,
        const SQLite::Version& version,
        const std::vector<V2_0::PackageUpdateTrackingTable::PackageData>& changedPackages,
        const std::set<SQLite::rowid_t>& removedPackages);
}
