// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Microsoft/PinningIndex.h>
#include <Microsoft/Schema/IPinningIndex.h>
#include <Microsoft/Schema/Pinning_1_0/PinTable.h>
#include <winget/Pin.h>

using namespace std::string_literals;
using namespace TestCommon;
using namespace AppInstaller::Pinning;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::SQLite;
using namespace AppInstaller::Repository::Microsoft::Schema;

TEST_CASE("PinningIndexCreateLatestAndReopen", "[pinningIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Version versionCreated;

    // Create the index
    {
        PinningIndex index = PinningIndex::CreateNew(tempFile, Version::Latest());
        versionCreated = index.GetVersion();
    }

    // Reopen the index for read only
    {
        INFO("Trying with Read");
        PinningIndex index = PinningIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Read);
        Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for read/write
    {
        INFO("Trying with ReadWrite");
        PinningIndex index = PinningIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }

    // Reopen the index for immutable read
    {
        INFO("Trying with Immutable");
        PinningIndex index = PinningIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::Immutable);
        Version versionRead = index.GetVersion();
        REQUIRE(versionRead == versionCreated);
    }
}

TEST_CASE("PinningIndexAddEntryToTable", "[pinningIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Pin pin = Pin::CreateBlockingPin({ "pkgId", "sourceId" });

    {
        PinningIndex index = PinningIndex::CreateNew(tempFile, { 1, 0 });
        index.AddPin(pin);
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);

        auto pins = Pinning_V1_0::PinTable::GetAllPins(connection);
        REQUIRE(pins.size() == 1);
        REQUIRE(pins[0] == pin);

        auto pinFromIndex = Pinning_V1_0::PinTable::GetPinById(connection, 1);
        REQUIRE(pinFromIndex.has_value());
        REQUIRE(pinFromIndex.value() == pin);

        REQUIRE(pinFromIndex->GetType() == pin.GetType());
        REQUIRE(pinFromIndex->GetKey() == pin.GetKey());
    }

    {
        PinningIndex index = PinningIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.RemovePin(pin.GetKey());
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(Pinning_V1_0::PinTable::GetAllPins(connection).empty());
        REQUIRE(!Pinning_V1_0::PinTable::GetPinById(connection, 1));
    }
}

TEST_CASE("PinningIndex_AddUpdateRemove", "[pinningIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Pin pin = Pin::CreateGatingPin({ "pkgId", "srcId" }, { "1.0.*"sv });
    Pin updatedPin = Pin::CreatePinningPin({ "pkgId", "srcId" });

    {
        PinningIndex index = PinningIndex::CreateNew(tempFile, { 1, 0 });
        index.AddPin(pin);
        REQUIRE(index.UpdatePin(updatedPin));
    }

    {
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadOnly);
        auto pinFromIndex = Pinning_V1_0::PinTable::GetPinById(connection, 1);
        REQUIRE(pinFromIndex.has_value());
        REQUIRE(pinFromIndex.value() == updatedPin);
    }

    {
        PinningIndex index = PinningIndex::Open(tempFile, SQLiteStorageBase::OpenDisposition::ReadWrite);
        index.RemovePin(updatedPin.GetKey());
    }

    {
        // Open it directly to directly test table state
        Connection connection = Connection::Create(tempFile, Connection::OpenDisposition::ReadWrite);
        REQUIRE(Pinning_V1_0::PinTable::GetAllPins(connection).empty());
        REQUIRE(!Pinning_V1_0::PinTable::GetPinById(connection, 1));
    }
}

TEST_CASE("PinningIndex_ResetAll", "[pinningIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Pin pin1 = Pin::CreateBlockingPin({ "pkg1", "src1" });
    Pin pin2 = Pin::CreatePinningPin({ "pkg2", "src2" });

    // Add two pins to the index, then check that they show up when queried
    PinningIndex index = PinningIndex::CreateNew(tempFile, { 1, 0 });
    index.AddPin(pin1);
    index.AddPin(pin2);

    REQUIRE(index.GetAllPins().size() == 2);
    REQUIRE(index.GetPin(pin1.GetKey()).has_value());
    REQUIRE(index.GetPin(pin2.GetKey()).has_value());
    REQUIRE(!index.GetPin({ "pkg", "src" }).has_value());

    // Reset the index, then check that there are no pins
    index.ResetAllPins();
    REQUIRE(index.GetAllPins().empty());
    REQUIRE(!index.GetPin(pin1.GetKey()).has_value());
    REQUIRE(!index.GetPin(pin2.GetKey()).has_value());
}

TEST_CASE("PinningIndex_AddDuplicatePin", "[pinningIndex]")
{
    TempFile tempFile{ "repolibtest_tempdb"s, ".db"s };
    INFO("Using temporary file named: " << tempFile.GetPath());

    Pin pin = Pin::CreateGatingPin({ "pkg", "src" }, { "1.*"sv });

    PinningIndex index = PinningIndex::CreateNew(tempFile, { 1, 0 });
    index.AddPin(pin);

    REQUIRE_THROWS(index.AddPin(pin), ERROR_ALREADY_EXISTS);
}