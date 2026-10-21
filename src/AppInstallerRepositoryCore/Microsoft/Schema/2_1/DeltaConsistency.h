// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/ISQLiteIndex.h"

#include <winget/SQLiteWrapper.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    // Checks a delta database that is being read without its baseline.
    //
    // Only a fraction of what a full index promises can be established here: a delta describes
    // change, and most of what it says is a statement about rows that live in the baseline. What
    // remains is everything the delta says about itself.
    bool CheckConsistency(const SQLite::Connection& connection, bool log);

    // Determines whether two indexes present the same data through the read interface.
    //
    // Comparing through the interface rather than over the tables is what makes this meaningful:
    // it is the surface that a client reads, and it is blind to how either side stores the data.
    bool CheckEquivalence(
        const ISQLiteIndex& first,
        const SQLite::Connection& firstConnection,
        const ISQLiteIndex& second,
        const SQLite::Connection& secondConnection,
        bool log);
}
