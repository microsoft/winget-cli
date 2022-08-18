// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/Portable_1_0/PortableIndexInterface.h"
#include "Microsoft/Schema/Portable_1_0/PortableTable.h"

namespace AppInstaller::Repository::Microsoft::Schema::Portable_V1_0
{
    namespace
    {
        std::optional<SQLite::rowid_t> GetExistingPortableFileId(const SQLite::Connection& connection, const PortableFile& file)
        {
            auto result = PortableTable::SelectByFilePath(connection, file.FilePath);

            if (!result)
            {
                AICLI_LOG(Repo, Verbose, << "Did not find a portable row for { " << file.FilePath << " }");
            }

            return result;
        }
    }

    Schema::Version PortableIndexInterface::GetVersion() const
    {
        return { 1, 0 };
    }

    void PortableIndexInterface::CreateTable(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createportableindextable_v1_0");
        Portable_V1_0::PortableTable::Create(connection);
        savepoint.Commit();
    }

    SQLite::rowid_t PortableIndexInterface::AddPortableFile(SQLite::Connection& connection, const PortableFile& file)
    {
        auto portableEntryResult = GetExistingPortableFileId(connection, file);

        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS), portableEntryResult.has_value());

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "addportableentry_v1_0");
        SQLite::rowid_t portableFileId = PortableTable::AddPortableFile(connection, file);

        savepoint.Commit();
        return portableFileId;
    }

    SQLite::rowid_t PortableIndexInterface::RemovePortableFile(SQLite::Connection& connection, const PortableFile& file)
    {
        auto portableEntryResult = GetExistingPortableFileId(connection, file);

        // If the portable file doesn't actually exist, fail the remove.
        THROW_HR_IF(E_NOT_SET, !portableEntryResult);

        PortableTable::RemovePortableFileById(connection, portableEntryResult.value());

        return portableEntryResult.value();
    }

    std::pair<bool, SQLite::rowid_t> PortableIndexInterface::UpdatePortableFile(SQLite::Connection& connection, const PortableFile& file)
    {
        auto portableEntryResult = GetExistingPortableFileId(connection, file);

        // If the portable file doesn't actually exist, fail the remove.
        THROW_HR_IF(E_NOT_SET, !portableEntryResult);

        bool status = PortableTable::UpdatePortableFileById(connection, portableEntryResult.value(), file);
        return { status, portableEntryResult.value() };
    }
}