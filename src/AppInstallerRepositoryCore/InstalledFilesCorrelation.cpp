// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/InstalledFilesCorrelation.h"
#include <winget/FolderFileWatcher.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Correlation
{
    namespace
    {
        constexpr std::string_view s_FileExtensionToWatch = ".lnk"sv;
    }

    InstalledFilesCorrelation::InstalledFilesCorrelation()
    {
        m_fileWatchers.emplace_back(Runtime::GetKnownFolderPath(FOLDERID_CommonStartMenu), std::string{ s_FileExtensionToWatch });
        m_fileWatchers.emplace_back(Runtime::GetKnownFolderPath(FOLDERID_StartMenu), std::string{ s_FileExtensionToWatch });
    }

    void InstalledFilesCorrelation::StartFileWatcher()
    {
        m_files.clear();

        for (auto& watcher : m_fileWatchers)
        {
            watcher.Start();
        }
    }

    void InstalledFilesCorrelation::StopFileWatcher()
    {
        for (auto& watcher : m_fileWatchers)
        {
            watcher.Stop();
            m_files.insert(m_files.end(), watcher.Files().begin(), watcher.Files().end());
        }
    }

    std::optional<AppInstaller::Manifest::InstallationMetadataInfo> InstalledFilesCorrelation::CorrelateForNewlyInstalled(
        const Manifest::Manifest&,
        const std::string&,
        const std::string&)
    {
        return {};
    }
}
