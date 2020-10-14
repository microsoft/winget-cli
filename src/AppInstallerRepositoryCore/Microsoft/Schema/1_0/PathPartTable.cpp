// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PathPartTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    using namespace std::string_view_literals;
    static constexpr std::string_view s_PathPartTable_Table_Name = "pathparts"sv;
    static constexpr std::string_view s_PathPartTable_PrimaryKeyIndex_Name = "pathparts_pkindex"sv;
    static constexpr std::string_view s_PathPartTable_ParentIndex_Name = "pathparts_parentidx"sv;
    static constexpr std::string_view s_PathPartTable_ParentValue_Name = "parent"sv;
    static constexpr std::string_view s_PathPartTable_PartValue_Name = "pathpart"sv;

    namespace
    {
        // Attempts to select a path part given the input.
        // Returns an no value if none exists, or the rowid of the part if it is found.
        std::optional<SQLite::rowid_t> SelectPathPart(SQLite::Connection& connection, std::optional<SQLite::rowid_t> parent, std::string_view part)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(SQLite::RowIDName).From(s_PathPartTable_Table_Name).
                Where(s_PathPartTable_ParentValue_Name).Equals(parent).And(s_PathPartTable_PartValue_Name).Equals(part);

            SQLite::Statement select = builder.Prepare(connection);

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

            SQLite::Builder::StatementBuilder builder;
            builder.InsertInto(s_PathPartTable_Table_Name).Columns({ s_PathPartTable_ParentValue_Name, s_PathPartTable_PartValue_Name }).Values(parent, part);

            builder.Execute(connection);

            return connection.GetLastInsertRowID();
        }

        // Gets the parent of a given part by id.
        // This should only be called when the part must exist, as it will throw if not found.
        std::optional<SQLite::rowid_t> GetParentById(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.Select(s_PathPartTable_ParentValue_Name).From(s_PathPartTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

            SQLite::Statement select = builder.Prepare(connection);

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
            SQLite::Builder::StatementBuilder builder;
            builder.Select(SQLite::Builder::RowCount).From(s_PathPartTable_Table_Name).Where(s_PathPartTable_ParentValue_Name).Equals(id);

            SQLite::Statement select = builder.Prepare(connection);

            THROW_HR_IF(E_UNEXPECTED, !select.Step());

            // No rows with this as a parent means it is a leaf.
            return (select.GetColumn<int>(0) == 0);
        }

        // Removes the given part by id.
        void RemovePartById(SQLite::Connection& connection, SQLite::rowid_t id)
        {
            SQLite::Builder::StatementBuilder builder;
            builder.DeleteFrom(s_PathPartTable_Table_Name).Where(SQLite::RowIDName).Equals(id);

            builder.Execute(connection);
        }
    }

    // Starting in V1.1, all code should be going this route of creating named indeces rather than using primary or unique keys on columns.
    // The resulting database will function the same, but give us control to drop the indeces to reduce space.
    void PathPartTable::Create(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createPathParts_v1_1");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_PathPartTable_Table_Name).Columns({
            IntegerPrimaryKey(),
            ColumnBuilder(s_PathPartTable_ParentValue_Name, Type::Int64),
            ColumnBuilder(s_PathPartTable_PartValue_Name, Type::Text).NotNull()
            });

        createTableBuilder.Execute(connection);

        StatementBuilder createPKIndexBuilder;
        createPKIndexBuilder.CreateUniqueIndex(s_PathPartTable_PrimaryKeyIndex_Name).On(s_PathPartTable_Table_Name).Columns({ s_PathPartTable_PartValue_Name, s_PathPartTable_ParentValue_Name });
        createPKIndexBuilder.Execute(connection);

        StatementBuilder createIndexBuilder;
        createIndexBuilder.CreateIndex(s_PathPartTable_ParentIndex_Name).On(s_PathPartTable_Table_Name).Columns(s_PathPartTable_ParentValue_Name);
        createIndexBuilder.Execute(connection);

        savepoint.Commit();
    }

    void PathPartTable::Create_deprecated(SQLite::Connection& connection)
    {
        using namespace SQLite::Builder;

        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "createPathParts_v1_0");

        StatementBuilder createTableBuilder;
        createTableBuilder.CreateTable(s_PathPartTable_Table_Name).Columns({
            ColumnBuilder(s_PathPartTable_ParentValue_Name, Type::Int64),
            ColumnBuilder(s_PathPartTable_PartValue_Name, Type::Text).NotNull(),
            PrimaryKeyBuilder({ s_PathPartTable_PartValue_Name, s_PathPartTable_ParentValue_Name })
            });

        createTableBuilder.Execute(connection);

        StatementBuilder createIndexBuilder;
        createIndexBuilder.CreateIndex(s_PathPartTable_ParentIndex_Name).On(s_PathPartTable_Table_Name).Columns(s_PathPartTable_ParentValue_Name);

        createIndexBuilder.Execute(connection);

        savepoint.Commit();
    }

    std::string_view PathPartTable::TableName()
    {
        return s_PathPartTable_Table_Name;
    }

    std::string_view PathPartTable::ValueName()
    {
        return s_PathPartTable_PartValue_Name;
    }

    std::tuple<bool, SQLite::rowid_t> PathPartTable::EnsurePathExists(SQLite::Connection& connection, const std::filesystem::path& relativePath, bool createIfNotFound)
    {
        THROW_HR_IF(E_INVALIDARG, !relativePath.has_relative_path());
        THROW_HR_IF(E_INVALIDARG, relativePath.has_root_path());
        THROW_HR_IF(E_INVALIDARG, !relativePath.has_filename());

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

    std::optional<std::string> PathPartTable::GetPathById(const SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select({ s_PathPartTable_ParentValue_Name, s_PathPartTable_PartValue_Name }).
            From(s_PathPartTable_Table_Name).Where(SQLite::RowIDName).Equals(SQLite::Builder::Unbound);

        SQLite::Statement select = builder.Prepare(connection);

        SQLite::rowid_t currentPart = id;
        std::string result;

        while (true)
        {
            select.Reset();
            select.Bind(1, currentPart);

            if (select.Step())
            {
                std::string partValue = select.GetColumn<std::string>(1);
                if (result.empty())
                {
                    result = partValue;
                }
                else
                {
                    result = partValue + '/' + result;
                }

                if (select.GetColumnIsNull(0))
                {
                    // If the parent of this column is null, then we have reached the relative root
                    break;
                }
                else
                {
                    currentPart = select.GetColumn<SQLite::rowid_t>(0);
                }
            }
            else
            {
                if (currentPart == id)
                {
                    // The given id did not reference an actual path
                    return {};
                }
                else
                {
                    // We found a broken path
                    AICLI_LOG(Repo, Error, << "Path part references an invalid parent: " << currentPart);
                    THROW_HR(APPINSTALLER_CLI_ERROR_INDEX_INTEGRITY_COMPROMISED);
                }
            }
        }

        return result;
    }

    void PathPartTable::RemovePathById(SQLite::Connection& connection, SQLite::rowid_t id)
    {
        SQLite::rowid_t currentPartToRemove = id;
        while (IsLeafPart(connection, currentPartToRemove))
        {
            std::optional<SQLite::rowid_t> parent = GetParentById(connection, currentPartToRemove);
            RemovePartById(connection, currentPartToRemove);

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

    void PathPartTable::PrepareForPackaging(SQLite::Connection& connection)
    {
        SQLite::Savepoint savepoint = SQLite::Savepoint::Create(connection, "pfpPathParts_v1_1");

        PrepareForPackaging_deprecated(connection);

        SQLite::Builder::StatementBuilder dropPKIndexBuilder;
        dropPKIndexBuilder.DropIndex(s_PathPartTable_PrimaryKeyIndex_Name);
        dropPKIndexBuilder.Execute(connection);

        savepoint.Commit();
    }

    void PathPartTable::PrepareForPackaging_deprecated(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder dropIndexBuilder;
        dropIndexBuilder.DropIndex(s_PathPartTable_ParentIndex_Name);
        dropIndexBuilder.Execute(connection);
    }

    bool PathPartTable::CheckConsistency(const SQLite::Connection& connection, bool log)
    {
        using QCol = SQLite::Builder::QualifiedColumn;

        // Build a select statement to find pathpart rows containing references to parents with non-existent rowids
        // Such as:
        // Select l.rowid, l.parent from pathparts as l left outer join pathparts as r on l.parent = r.rowid where l.parent is not null and r.pathpart is null
        constexpr std::string_view s_left = "left"sv;
        constexpr std::string_view s_right = "right"sv;

        SQLite::Builder::StatementBuilder builder;
        builder.
            Select({ QCol(s_left, SQLite::RowIDName), QCol(s_left, s_PathPartTable_ParentValue_Name) }).
            From(s_PathPartTable_Table_Name).As(s_left).
            LeftOuterJoin(s_PathPartTable_Table_Name).As(s_right).On(QCol(s_left, s_PathPartTable_ParentValue_Name), QCol(s_right, SQLite::RowIDName)).
            Where(QCol(s_left, s_PathPartTable_ParentValue_Name)).IsNotNull().And(QCol(s_right, s_PathPartTable_PartValue_Name)).IsNull();

        SQLite::Statement select = builder.Prepare(connection);
        bool result = true;

        while (select.Step())
        {
            result = false;

            if (!log)
            {
                break;
            }

            AICLI_LOG(Repo, Info, << "  [INVALID] pathparts [" << select.GetColumn<SQLite::rowid_t>(0) << "] refers to " << s_PathPartTable_ParentValue_Name << " [" << select.GetColumn<SQLite::rowid_t>(1) << "]");
        }

        return result;
    }

    bool PathPartTable::IsEmpty(SQLite::Connection& connection)
    {
        SQLite::Builder::StatementBuilder builder;
        builder.Select(SQLite::Builder::RowCount).From(s_PathPartTable_Table_Name);

        SQLite::Statement countStatement = builder.Prepare(connection);

        THROW_HR_IF(E_UNEXPECTED, !countStatement.Step());

        return (countStatement.GetColumn<int>(0) == 0);
    }
}
