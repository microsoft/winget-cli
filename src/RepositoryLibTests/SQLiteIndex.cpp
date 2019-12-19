// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/SQLiteIndex.h>

using namespace AppInstaller::Repository::Microsoft;

TEST_CASE("SQLiteIndexCreateLatestAndReopen", "[sqliteindex]")
{
    TestCommon::TempFile tempFile{ "repolibtest_tempdb", ".db" };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Schema::Version versionCreated;

    // Create the index
    {
        SQLiteIndex index = SQLiteIndex::CreateNew(tempFile, Schema::Version::Latest());
        versionCreated = index.GetVersion();
    }

    // Reopen the index for read only
    {
        INFO("Trying with Read");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::Read);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for read/write
    {
        INFO("Trying with ReadWrite");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::ReadWrite);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for immutable read
    {
        INFO("Trying with Immutable");
        SQLiteIndex index = SQLiteIndex::Open(tempFile, SQLiteIndex::OpenDisposition::Immutable);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}
