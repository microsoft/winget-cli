// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
#include <AppInstallerMsixInfo.h>
namespace AppInstaller::Repository::Microsoft
{
    // Holds a temp SQLite index file extracted from index package, exclusive to the process.
    struct TempSQLiteIndexFile
    {
        TempSQLiteIndexFile() = default;

        TempSQLiteIndexFile(AppInstaller::Msix::MsixInfo& packageInfo, AppInstaller::IProgressCallback& progress);
        
        TempSQLiteIndexFile(const TempSQLiteIndexFile&) = delete;
        TempSQLiteIndexFile& operator=(const TempSQLiteIndexFile&) = delete;

        TempSQLiteIndexFile(TempSQLiteIndexFile&&) = default;
        TempSQLiteIndexFile& operator=(TempSQLiteIndexFile&&) = default;

        const std::filesystem::path& GetIndexFilePath() { return m_indexFile; }

        ~TempSQLiteIndexFile();

    private:
        std::filesystem::path m_indexFile;
        wil::unique_handle m_indexHandle;
    };
}
