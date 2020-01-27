// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PathPartTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PathPartTable_Table_Name = "pathparts"sv;
    static constexpr std::string_view s_PathPartTable_ParentValue_Name = "parent"sv;
    static constexpr std::string_view s_PathPartTable_PartValue_Name = "pathpart"sv;

    namespace
    {
        // Attempts to select a path part given the input.
        // Returns an no value if none exists, or the rowid of the part if it is found.
        std::optional<SQLite::rowid_t> SelectPathPart(SQLite::Connection& connection, std::optional<SQLite::rowid_t> parent, std::string_view part)
        {
            std::ostringstream selectPartSQL;
            selectPartSQL << "SELECT [" << SQLite::RowIDName << "] "
                << "FROM [" << s_PathPartTable_Table_Name << "] WHERE "
                << '[' << s_PathPartTable_ParentValue_Name << "] " << (parent ? "= ?" : "IS NULL") << " AND "
                << '[' << s_PathPartTable_PartValue_Name << "] = ?";

            SQLite::Statement select = SQLite::Statement::Create(connection, selectPartSQL.str());

            if (parent)
            {
                select.Bind(1, parent.value());
                select.Bind(2, part);
            }
            else
            {
                select.Bind(1, part);
            }

            if (select.Step())
            {
                return select.GetColumn<SQLite::rowid_t>(0);
            }
            else
            {
                return {};
            }
        }

        // Inserts the given path part into the table, returning the rowid of the inserted row.
        SQLite::rowid_t InsertPathPart(SQLite::Connection& connection, std::optional<SQLite::rowid_t> parent, std::string_view part)
        {
            THROW_HR_IF(E_INVALIDARG, part.empty());

            std::ostringstream insertPartSQL;
            insertPartSQL << "INSERT INTO [" << s_PathPartTable_Table_Name << "] ("
                << '[' << s_PathPartTable_ParentValue_Name << "],"
                << '[' << s_PathPartTable_PartValue_Name << "])"
                << " VALUES (?, ?)";

            SQLite::Statement insert = SQLite::Statement::Create(connection, insertPartSQL.str());

            if (parent)
            {
                insert.Bind(1, parent.value());
            }
            else
            {
                insert.Bind(1, nullptr);
            }
            insert.Bind(2, part);

            insert.Execute();

            return connection.GetLastInsertRowID();
        }
    }

    void PathPartTable::Create(SQLite::Connection& connection)
    {
        std::ostringstream createTableSQL;
        createTableSQL << "CREATE TABLE [" << s_PathPartTable_Table_Name << "]("
            << '[' << s_PathPartTable_ParentValue_Name << "] INT64,"
            << '[' << s_PathPartTable_PartValue_Name << "] TEXT NOT NULL,"
            << "PRIMARY KEY([" << s_PathPartTable_PartValue_Name << "], [" << s_PathPartTable_ParentValue_Name << "]))";

        SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

        createStatement.Execute();
    }

    std::string_view PathPartTable::ValueName()
    {
        return s_PathPartTable_PartValue_Name;
    }

    std::tuple<bool, SQLite::rowid_t> PathPartTable::EnsurePathExists(SQLite::Connection& connection, const std::filesystem::path& relativePath, bool createIfNotFound)
    {
        THROW_HR_IF(E_INVALIDARG, !relativePath.has_relative_path());
        THROW_HR_IF(E_INVALIDARG, relativePath.has_root_path());

        std::unique_ptr<SQLite::Savepoint> savepoint;
        if (createIfNotFound)
        {
            savepoint = std::make_unique<SQLite::Savepoint>(SQLite::Savepoint::Create(connection, "ensurepathexists"));
        }

        bool partsAdded = false;

        std::optional<SQLite::rowid_t> parent;
        for (const auto& part : relativePath)
        {
            std::string utf8part = part.u8string();
            std::optional<SQLite::rowid_t> current = SelectPathPart(connection, parent, utf8part);

            if (!current)
            {
                if (createIfNotFound)
                {
                    partsAdded = true;
                    current = InsertPathPart(connection, parent, utf8part);
                }
                else
                {
                    // Current part was not found, and we were told not to create.
                    // Return false to indicate that the path does not exist.
                    return {};
                }
            }

            parent = current;
        }

        if (savepoint)
        {
            savepoint->Commit();
        }

        // If we get this far, the path exists.
        // If we were asked to create it, return whether we needed to or it was already present.
        // If not, then true indicates that it exists.
        return { (createIfNotFound ? partsAdded : true), parent.value() };
    }
}
