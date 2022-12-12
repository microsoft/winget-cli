#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/IPinningIndex.h"
#include "Microsoft/SQLiteStorageBase.h"
#include "winget/Pin.h"
#include <winget/ManagedFile.h>

namespace AppInstaller::Repository::Microsoft
{
    struct PinningIndex : SQLiteStorageBase
    {
        // An id that refers to a specific Pinning file.
        using IdType = SQLite::rowid_t;

        PinningIndex(const PinningIndex&) = delete;
        PinningIndex& operator=(const PinningIndex&) = delete;

        PinningIndex(PinningIndex&&) = default;
        PinningIndex& operator=(PinningIndex&&) = default;

        // Creates a new PinningIndex database of the given version.
        static PinningIndex CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        // Opens an existing PinningIndex database.
        static PinningIndex Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile = {})
        {
            return { filePath, disposition, std::move(indexFile) };
        }

        // Adds a pin to the index.
        IdType AddPin(const Pinning::Pin& pin);

        // Removes a pin from the index.
        void RemovePin(const Pinning::PinKey& pinKey);

        // Returns the current pin for a given package if it exists.
        std::optional<Pinning::Pin> GetPin(const Pinning::PinKey& pinKey);

        // Returns a vector containing all the existing pins.
        std::vector<Pinning::Pin> GetAllPins(SQLite::Connection& connection);

    private:
        // Constructor used to open an existing index.
        PinningIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        PinningIndex(const std::string& target, Schema::Version version);

        // Creates the IPinningIndex interface object for this version.
        std::unique_ptr<Schema::IPinningIndex> CreateIPinningIndex() const;

        std::unique_ptr<Schema::IPinningIndex> m_interface;
    };
}