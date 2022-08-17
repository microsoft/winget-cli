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

using namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0;

void CreateFakePortableFile(PortableFile& file)
{
    file.FilePath = "testFilePath.exe";
    file.FileType = FileTypeEnum::File;
    file.SHA256 = "91827349812739847928134";
    file.SymlinkTarget = "testSymlinkTarget.exe";
    file.IsCreated = true;
}

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
    {
        INFO("Trying with Read");
        PortableIndex index = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::Read);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    //// Reopen the index for read/write
    {
        INFO("Trying with ReadWrite");
        PortableIndex index = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::ReadWrite);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    //// Reopen the index for immutable read
    {
        INFO("Trying with Immutable");
        PortableIndex index = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::Immutable);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("PortableIndexAddEntryToTable", "[portableindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    PortableFile portableFile;
    portableFile.FilePath = "testFilePath.exe";
    portableFile.FileType = FileTypeEnum::File;
    portableFile.SHA256 = "91827349812739847928134";
    portableFile.SymlinkTarget = "testSymlinkTarget.exe";
    portableFile.IsCreated = true;

    {
        PortableIndex index = PortableIndex::CreateNew(tempFile, { 1, 0 });
        index.AddPortableFile(portableFile);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(!Schema::Portable_V1_0::PortableTable::IsEmpty(connection));
    }

    {
        PortableIndex index = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::ReadWrite);
        index.RemovePortableFile(portableFile);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(Schema::Portable_V1_0::PortableTable::IsEmpty(connection));
    }
}

TEST_CASE("PortableIndex_AddUpdateRemove", "[portableindex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    PortableFile portableFile;
    portableFile.FilePath = "testFilePath.exe";
    portableFile.FileType = FileTypeEnum::File;
    portableFile.SHA256 = "91827349812739847928134";
    portableFile.SymlinkTarget = "testSymlinkTarget.exe";
    portableFile.IsCreated = true;

    PortableIndex index = PortableIndex::CreateNew(tempFile, { 1, 0 });
    index.AddPortableFile(portableFile);

    // Apply changes
    portableFile.FileType = FileTypeEnum::Symlink;
    portableFile.SHA256 = "0928340928304982309";
    portableFile.SymlinkTarget = "fakeSymlinkTarget.exe";
    portableFile.IsCreated = false;

    REQUIRE(index.UpdatePortableFile(portableFile));

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        auto fileFromIndex = Schema::Portable_V1_0::PortableTable::GetPortableFileById(connection, 1);
        REQUIRE(fileFromIndex.has_value());
        REQUIRE(fileFromIndex->FilePath == "testFilePath.exe");
        REQUIRE(fileFromIndex->FileType == FileTypeEnum::Symlink);
        REQUIRE(fileFromIndex->SHA256 == "0928340928304982309");
        REQUIRE(fileFromIndex->SymlinkTarget == "fakeSymlinkTarget.exe");
        REQUIRE(fileFromIndex->IsCreated == false);
    }

    {
        PortableIndex index2 = PortableIndex::Open(tempFile, PortableIndex::OpenDisposition::ReadWrite);
        index2.RemovePortableFile(portableFile);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(Schema::Portable_V1_0::PortableTable::IsEmpty(connection));
    }
}