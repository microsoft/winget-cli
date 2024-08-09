// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/FolderFileWatcher.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    FolderFileWatcher::FolderFileWatcher(const std::filesystem::path& path, const std::optional<std::string>& ext) :
        m_path(path), m_ext(ext), m_changeReader{}
    {
    }

    void FolderFileWatcher::Start()
    {
        m_files.clear();
        m_changeReader = wil::make_folder_change_reader(m_path.c_str(),
            true,
            wil::FolderChangeEvents::FileName,
            [this](wil::FolderChangeEvent changeEvent, PCWSTR filePath)
            {
                switch (changeEvent)
                {
                // The file was added to the directory.
                case wil::FolderChangeEvent::Added:
                // The file was renamed and this is the new name.
                case wil::FolderChangeEvent::RenameNewName:
                {
                    std::filesystem::path path(filePath);
                    if (!m_ext.has_value() || Utility::CaseInsensitiveEquals(path.extension().u8string(), *m_ext))
                    {
                        m_files.emplace(path);
                    }
                    break;
                }

                // The file was removed from the directory.
                case wil::FolderChangeEvent::Removed:
                // The file was renamed and this is the old name.
                case wil::FolderChangeEvent::RenameOldName:
                {
                    std::filesystem::path path(filePath);
                    if (!m_ext.has_value() || Utility::CaseInsensitiveEquals(path.extension().u8string(), *m_ext))
                    {
                        auto it = m_files.find(path);
                        if (it != m_files.cend())
                        {
                            m_files.erase(it);
                        }
                    }
                    break;
                }

                // The file was modified. This can be a change in the time stamp or attributes.
                case wil::FolderChangeEvent::Modified:
                // A change happens but it got lost. The result of the IoCompletionCallback is ERROR_NOTIFY_ENUM_DIR.
                case wil::FolderChangeEvent::ChangesLost:
                default:
                    break;
                }
            });
    }

    void FolderFileWatcher::Stop()
    {
        m_changeReader.reset();
    }
}