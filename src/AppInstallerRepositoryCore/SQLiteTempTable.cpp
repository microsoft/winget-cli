// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SQLiteTempTable.h"
#include "SQLiteStatementBuilder.h"


namespace AppInstaller::Repository::SQLite
{
    TempTable::TempTable()
    {
        GUID tempName;
        THROW_IF_FAILED(CoCreateGuid(&tempName));

        wchar_t guidAsString[MAX_PATH];
        THROW_HR_IF(E_UNEXPECTED, StringFromGUID2(tempName, guidAsString, MAX_PATH) == 0);

        m_name = "[temp].[";
        m_name += Utility::ConvertToUTF8(guidAsString);
        m_name += ']';
    }

    TempTable::~TempTable()
    {
        if (m_dropTableStatement)
        {
            m_dropTableStatement.Execute();
        }
    }

    void TempTable::InitDropStatement(Connection& connection)
    {
        Builder::StatementBuilder builder;
        builder.DropTable(m_name);
        
        m_dropTableStatement = builder.Prepare(connection);
    }
}
