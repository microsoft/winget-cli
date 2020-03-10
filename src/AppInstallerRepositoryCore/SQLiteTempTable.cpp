// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "SQLiteTempTable.h"


namespace AppInstaller::Repository::SQLite
{
    using namespace std::string_view_literals;

    TempTable::TempTable()
    {
        GUID tempName;
        THROW_IF_FAILED(CoCreateGuid(&tempName));

        wchar_t guidAsString[MAX_PATH];
        THROW_HR_IF(E_UNEXPECTED, StringFromGUID2(tempName, guidAsString, MAX_PATH) == 0);

        m_name = Utility::ConvertToUTF8(guidAsString);
    }

    TempTable::~TempTable()
    {
        if (m_dropTableStatement)
        {
            m_dropTableStatement.Execute();
        }
    }

    Builder::QualifiedTable TempTable::GetQualifiedName() const
    {
        return Builder::QualifiedTable("temp"sv, m_name);
    }

    void TempTable::InitDropStatement(Connection& connection)
    {
        Builder::StatementBuilder builder;
        builder.DropTable(m_name);
        
        m_dropTableStatement = builder.Prepare(connection);
    }
}
