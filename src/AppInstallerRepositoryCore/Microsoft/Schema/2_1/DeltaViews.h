// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/SQLiteWrapper.h>


namespace AppInstaller::Repository::Microsoft::Schema::V2_1::Delta
{
    // Attaches the given baseline database to the delta database that the connection is open on,
    // then defines a temporary view for each 2.0 table that presents the combination of the two.
    //
    // The views take the 2.0 table names for themselves, and the delta's own tables are named
    // distinctly, so every 2.0 read path operates on the merged data without knowing that it is
    // merged. The views are temporary, so they last only as long as the connection.
    void SetupReadMode(SQLite::Connection& connection, const SQLite::DatabaseSpecifier& baseline);
}
