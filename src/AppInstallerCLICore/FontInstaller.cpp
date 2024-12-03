// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "FontInstaller.h"
#include <winget/Manifest.h>
#include <winget/ManifestCommon.h>
#include <winget/Filesystem.h>
#include <AppInstallerErrors.h>
#include <AppInstallerRuntime.h>

namespace AppInstaller::CLI::Font
{
    namespace
    {
        constexpr std::wstring_view s_FontsPathSubkey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
    }


    FontInstaller::FontInstaller(Manifest::ScopeEnum scope) : m_scope(scope)
    {
        if (scope == Manifest::ScopeEnum::Machine)
        {
            m_installLocation = Runtime::GetPathTo(Runtime::PathName::FontsMachineInstallLocation);
            m_key = Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, std::wstring{ s_FontsPathSubkey });
        }
        else
        {
            m_installLocation = Runtime::GetPathTo(Runtime::PathName::FontsUserInstallLocation);
            m_key = Registry::Key::OpenIfExists(HKEY_CURRENT_USER, std::wstring{ s_FontsPathSubkey });
        }
    }

    void FontInstaller::Install(const std::map<std::wstring, std::filesystem::path> fontFiles)
    {
        // TODO: Get all font files and check if font file already exists.
        for (const auto& [fontName, fontFilePath] : fontFiles)
        {
            std::filesystem::path fileName = fontFilePath.filename();
            std::filesystem::path fontFileInstallationPath = m_installLocation / fileName;

            AICLI_LOG(CLI, Info, << "Moving font file to: " << fontFileInstallationPath.u8string());
            AppInstaller::Filesystem::RenameFile(fontFilePath, fontFileInstallationPath);

            // TODO: Need to fix access permission for writing to the registry.
            AICLI_LOG(CLI, Info, << "Creating font subkey for: " << AppInstaller::Utility::ConvertToUTF8(fontName));
            m_key.SetValue(fontName, fontFileInstallationPath, REG_SZ);
        }
    }
}
