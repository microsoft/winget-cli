// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <SQLiteWrapper.h>
#include <Microsoft/SQLiteStorageBase.h>
#include <Microsoft/Schema/IPortableIndex.h>
#include <Microsoft/PortableIndex.h>
#include <Microsoft/Schema/Portable_1_0/PortableTable.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Repository::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;

void CreateFakePortableFile(IPortableIndex::PortableFile& file)
{
    file.SetFilePath("testPortableFile.exe");
    file.FileType = IPortableIndex::PortableFileType::File;
    file.SHA256 = "f0e4c2f76c58916ec258f246851bea091d14d4247a2fc3e18694461b1816e13b";
    file.SymlinkTarget = "testSymlinkTarget.exe";
}

TEST_CASE("PortableIndexCreateLatestAndReopen", "[portableIndex]")
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
        PortableIndex index = PortableIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Read);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    //// Reopen the index for read/write
    {
        INFO("Trying with ReadWrite");
        PortableIndex index = PortableIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    //// Reopen the index for immutable read
    {
        INFO("Trying with Immutable");
        PortableIndex index = PortableIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Immutable);
        Schema::Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("PortableIndexAddEntryToTable", "[portableIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    IPortableIndex::PortableFile portableFile;
    CreateFakePortableFile(portableFile);

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
        PortableIndex index = PortableIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.RemovePortableFile(portableFile);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(Schema::Portable_V1_0::PortableTable::IsEmpty(connection));
    }
}

TEST_CASE("PortableIndex_AddUpdateRemove", "[portableIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    IPortableIndex::PortableFile portableFile;
    CreateFakePortableFile(portableFile);

    PortableIndex index = PortableIndex::CreateNew(tempFile, { 1, 0 });
    index.AddPortableFile(portableFile);

    // Apply changes to portable file
    std::string updatedHash = "2db8ae7657c6622b04700137740002c51c36588e566651c9f67b4b096c8ad18b";
    portableFile.FileType = IPortableIndex::PortableFileType::Symlink;
    portableFile.SHA256 = updatedHash;
    portableFile.SymlinkTarget = "fakeSymlinkTarget.exe";

    REQUIRE(index.UpdatePortableFile(portableFile));

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        auto fileFromIndex = Schema::Portable_V1_0::PortableTable::GetPortableFileById(connection, 1);
        REQUIRE(fileFromIndex.has_value());
        REQUIRE(fileFromIndex->GetFilePath() == "testPortableFile.exe");
        REQUIRE(fileFromIndex->FileType == IPortableIndex::PortableFileType::Symlink);
        REQUIRE(fileFromIndex->SHA256 == updatedHash);
        REQUIRE(fileFromIndex->SymlinkTarget == "fakeSymlinkTarget.exe");
    }

    {
        PortableIndex index2 = PortableIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index2.RemovePortableFile(portableFile);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(Schema::Portable_V1_0::PortableTable::IsEmpty(connection));
    }
}

TEST_CASE("PortableIndex_RemoveWithId", "[portableIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    IPortableIndex::PortableFile portableFile;
    CreateFakePortableFile(portableFile);

    PortableIndex index = PortableIndex::CreateNew(tempFile, { 1, 0 });
    index.AddPortableFile(portableFile);

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(Portable_V1_0::PortableTable::ExistsById(connection, 1));
        Portable_V1_0::PortableTable::DeleteById(connection, 1);
        REQUIRE_FALSE(Portable_V1_0::PortableTable::ExistsById(connection, 1));
    }
}