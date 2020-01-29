// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PathPartTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PathPartTable_Table_Name = "pathparts"sv;
    static constexpr std::string_view s_PathPartTable_ParentIndex_Name = "pathparts_parentidx"sv;
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

        // Gets the parent of a given part by id.
        // This should only be called when the part must exist, as it will throw if not found.
        std::optional<SQLite::rowid_t> GetParentById(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            std::ostringstream selectPartSQL;
            selectPartSQL << "SELECT [" << s_PathPartTable_Table_Name << "] FROM [" << s_PathPartTable_Table_Name << "] WHERE "
                << '[' << SQLite::RowIDName << "] = ?";

            SQLite::Statement select = SQLite::Statement::Create(connection, selectPartSQL.str());

            select.Bind(1, id);

            THROW_HR_IF(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED, !select.Step());

            if (!select.GetColumnIsNull(0))
            {
                return select.GetColumn<SQLite::rowid_t>(0);
            }
            else
            {
                return {};
            }
        }

        // Determines if any part references this one as their parent.
        bool IsLeafPart(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            std::ostringstream selectPartSQL;
            selectPartSQL << "SELECT COUNT(*) FROM [" << s_PathPartTable_Table_Name << "] WHERE "
                << '[' << s_PathPartTable_ParentValue_Name << "] = ?";

            SQLite::Statement select = SQLite::Statement::Create(connection, selectPartSQL.str());

            select.Bind(1, id);

            THROW_HR_IF(E_UNEXPECTED, !select.Step());

            // No rows with this as a parent means it is a leaf.
            return (select.GetColumn<int>(0) == 0);
        }

        // Removes the given part by id.
        bool RemovePartById(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            std::ostringstream deletePartSQL;
            deletePartSQL << "DELETE FROM [" << s_PathPartTable_Table_Name << "] WHERE "
                << '[' << SQLite::RowIDName << "] = ?";

            SQLite::Statement deletePart = SQLite::Statement::Create(connection, deletePartSQL.str());

            deletePart.Bind(1, id);

            deletePart.Execute();
        }
    }

    void PathPartTable::Create(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createPathParts_v1_0");

        {
            std::ostringstream createTableSQL;
            createTableSQL << "CREATE TABLE [" << s_PathPartTable_Table_Name << "]("
                << '[' << s_PathPartTable_ParentValue_Name << "] INT64,"
                << '[' << s_PathPartTable_PartValue_Name << "] TEXT NOT NULL,"
                << "PRIMARY KEY([" << s_PathPartTable_PartValue_Name << "], [" << s_PathPartTable_ParentValue_Name << "]))";

            SQLite::Statement createStatement = SQLite::Statement::Create(connection, createTableSQL.str());

            createStatement.Execute();
        }

        {
            std::ostringstream createIndexSQL;
            createIndexSQL << "CREATE INDEX [" << s_PathPartTable_ParentIndex_Name << "] "
                << "ON [" << s_PathPartTable_Table_Name << "]("
                << '[' << s_PathPartTable_ParentValue_Name << "])";

            SQLite::Statement createStatement = SQLite::Statement::Create(connection, createIndexSQL.str());

            createStatement.Execute();
        }

        savepoint.Commit();
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
            savepoint = std::make_unique<SQLite::Savepoint>(SQLite::Savepoint::Create(connection, "ensurepathexists_v1_0"));
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

    void PathPartTable::RemovePathById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::rowid_t currentPartToRemove = id;
        while (IsLeafPart(connection, id))
        {
            std::optional<SQLite::rowid_t> parent = GetParentById(connection, id);
            RemovePartById(connection, id);

            // If parent was NULL, this was a root part and we can stop
            if (!parent)
            {
                break;
            }
            else
            {
                currentPartToRemove = parent.value();
            }
        }
    }
}
