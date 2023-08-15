// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "Microsoft/Schema/ICheckpointIndex.h"
#include "Microsoft/SQLiteStorageBase.h"
#include <winget/ManagedFile.h>

namespace AppInstaller::Repository::Microsoft
{
    struct CheckpointIndex : SQLiteStorageBase
    {
        // An id that refers to a specific Checkpoint.
        using IdType = SQLite::rowid_t;

        CheckpointIndex(const CheckpointIndex&) = delete;
        CheckpointIndex& operator=(const CheckpointIndex&) = delete;

        CheckpointIndex(CheckpointIndex&&) = default;
        CheckpointIndex& operator=(CheckpointIndex&&) = default;

        // Creates a new CheckpointIndex database of the given version.
        static CheckpointIndex CreateNew(const std::string& filePath, Schema::Version version = Schema::Version::Latest());

        // Opens an existing CheckpointIndex database.
        static CheckpointIndex Open(const std::string& filePath, OpenDisposition disposition, Utility::ManagedFile&& indexFile = {})
        {
            return { filePath, disposition, std::move(indexFile) };
        }

        // Opens or creates a CheckpointIndex database on the default path.
        static std::shared_ptr<CheckpointIndex> OpenOrCreateDefault(GUID guid, OpenDisposition openDisposition = OpenDisposition::ReadWrite);

        // Gets the file path of the CheckpointIndex database.
        static std::filesystem::path GetCheckpointIndexPath(GUID guid);

        bool IsEmpty();

        IdType SetClientVersion(std::string_view clientVersion);

        std::string GetClientVersion();

        IdType SetCommandName(std::string_view commandName);

        std::string GetCommandName();

        IdType SetCommandArguments(std::string_view commandArguments);

        std::string GetCommandArguments();

        IdType AddContextData(std::string_view checkpointName, int contextData, std::string_view name, std::string_view value);

        std::string GetContextData(std::string_view checkpointName, int contextData, std::string_view name);

        void RemoveContextData(std::string_view checkpointName, int contextData);

    private:
        // Constructor used to open an existing index.
        CheckpointIndex(const std::string& target, SQLiteStorageBase::OpenDisposition disposition, Utility::ManagedFile&& indexFile);

        // Constructor used to create a new index.
        CheckpointIndex(const std::string& target, Schema::Version version);

        // Creates the ICheckpointIndex interface object for this version.
        std::unique_ptr<Schema::ICheckpointIndex> CreateICheckpointIndex() const;

        std::unique_ptr<Schema::ICheckpointIndex> m_interface;
    };
}