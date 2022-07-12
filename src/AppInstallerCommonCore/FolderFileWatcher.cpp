// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/FolderFileWatcher.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Utility
{
    FolderFileWatcher::FolderFileWatcher(const std::filesystem::path& path) :
        m_path(path), m_changeReader{}
    {
    }

    FolderFileWatcher::~FolderFileWatcher()
    {
        if (m_changeReader)
        {
            this->stop();
        }
    }

    void FolderFileWatcher::start()
    {
        m_files.clear();
        m_changeReader = wil::make_folder_change_reader(m_path.wstring().c_str(),
            true,
            wil::FolderChangeEvents::FileName,
            [this](wil::FolderChangeEvent changeEvent, PCWSTR path)
            {
                switch (changeEvent)
                {
                // The file was added to the directory.
                case wil::FolderChangeEvent::Added:
                // The file was renamed and this is the new name.
                case wil::FolderChangeEvent::RenameNewName:
                {
                    std::filesystem::path fspath = m_path / std::filesystem::path(path);
                    m_files.emplace(fspath.string());
                    break;
                }

                // The file was removed from the directory.
                case wil::FolderChangeEvent::Removed:
                // The file was renamed and this is the old name.
                case wil::FolderChangeEvent::RenameOldName:
                {
                    std::filesystem::path fspath = m_path / std::filesystem::path(path);
                    auto it = m_files.find(fspath.string());
                    if (it != m_files.cend())
                    {
                        m_files.erase(it);
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

    void FolderFileWatcher::stop()
    {
        m_changeReader.reset();
    }

    FolderFileExtensionWatcher::FolderFileExtensionWatcher(const std::filesystem::path& path, const std::string& ext) :
        m_path(path), m_ext(ext), m_changeReader{}
    {
    }

    FolderFileExtensionWatcher::~FolderFileExtensionWatcher()
    {
        if (m_changeReader)
        {
            this->stop();
        }
    }

    void FolderFileExtensionWatcher::start()
    {
        m_files.clear();
        m_changeReader = wil::make_folder_change_reader(m_path.wstring().c_str(),
            true,
            wil::FolderChangeEvents::FileName,
            [this](wil::FolderChangeEvent changeEvent, PCWSTR path)
            {
                switch (changeEvent)
                {
                // The file was added to the directory.
                case wil::FolderChangeEvent::Added:
                // The file was renamed and this is the new name.
                case wil::FolderChangeEvent::RenameNewName:
                {
                    std::filesystem::path fspath = m_path / std::filesystem::path(path);
                    if (fspath.extension() == m_ext)
                    {
                        m_files.emplace(fspath.string());
                    }
                    break;
                }

                // The file was removed from the directory.
                case wil::FolderChangeEvent::Removed:
                // The file was renamed and this is the old name.
                case wil::FolderChangeEvent::RenameOldName:
                {
                    std::filesystem::path fspath = m_path / std::filesystem::path(path);
                    if (fspath.extension() == m_ext)
                    {
                        auto it = m_files.find(fspath.string());
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

    void FolderFileExtensionWatcher::stop()
    {
        m_changeReader.reset();
    }
}