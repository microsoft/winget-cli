// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <SQLiteWrapper.h>
#include <Microsoft/PortableIndex.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Utility;


TEST_CASE("PortableIndexCreateLatestAndReopen", "[portableindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Schema::Version versionCreated;

    // Create the index
    {
        PortableIndex index = PortableIndex::CreateNew(tempFile, Schema::Version::Latest());
        versionCreated = index.GetVersion();
    }

    // Reopen the index for read only
    //{
    //    INFO("Trying with Read");
    //    PortableIndex index = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::Read);
    //    Schema::Version versionRead = index.GetVersion();
    //    REQUIRE(versionRead == versionCreated);
    //}

    //// Reopen the index for read/write
    //{
    //    INFO("Trying with ReadWrite");
    //    PortableIndex index = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::ReadWrite);
    //    Schema::Version versionRead = index.GetVersion();
    //    REQUIRE(versionRead == versionCreated);
    //}

    //// Reopen the index for immutable read
    //{
    //    INFO("Trying with Immutable");
    //    PortableIndex index = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::Immutable);
    //    Schema::Version versionRead = index.GetVersion();
    //    REQUIRE(versionRead == versionCreated);
    //}
}