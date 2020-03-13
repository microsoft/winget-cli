// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SQLiteWrapper.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::SQLite
{
    // The base for a class that represents a temp table.
    struct TempTable
    {
        TempTable();

        ~TempTable();

        TempTable(const TempTable&) = delete;
        TempTable& operator=(const TempTable&) = delete;

        TempTable(TempTable&&) = default;
        TempTable& operator=(TempTable&&) = default;

    protected:
        // Gets the qualified name of the temp table.
        Builder::QualifiedTable GetQualifiedName() const;

        // Prepares the drop table statement for use in destructor.
        // It needs to be run by the derived class after the table is actually created.
        void InitDropStatement(Connection& connection);

    private:
        std::string m_name;
        Statement m_dropTableStatement;
    };
}
