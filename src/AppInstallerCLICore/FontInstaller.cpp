// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ExecutionContext.h"
#include "FontInstaller.h"
#include <winget/Fonts.h>
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
        constexpr std::wstring_view s_TrueType = L" (TrueType)";

        bool IsTrueTypeFont(DWRITE_FONT_FILE_TYPE fileType)
        {
            return (
                fileType == DWRITE_FONT_FILE_TYPE_TRUETYPE ||
                fileType == DWRITE_FONT_FILE_TYPE_TRUETYPE_COLLECTION
                );
        }
    }

    FontInstaller::FontInstaller(Manifest::ScopeEnum scope) : m_scope(scope)
    {
        if (scope == Manifest::ScopeEnum::Machine)
        {
            m_installLocation = Runtime::GetPathTo(Runtime::PathName::FontsMachineInstallLocation);
            m_key = Registry::Key::Create(HKEY_LOCAL_MACHINE, std::wstring{ s_FontsPathSubkey });
        }
        else
        {
            m_installLocation = Runtime::GetPathTo(Runtime::PathName::FontsUserInstallLocation);
            m_key = Registry::Key::Create(HKEY_CURRENT_USER, std::wstring{ s_FontsPathSubkey });
        }
    }

    void FontInstaller::Install(const std::vector<FontFile>& fontFiles)
    {
        for (const auto& fontFile : fontFiles)
        {
            const auto& filePath = fontFile.FilePath;
            const auto& fileName = filePath.filename();
            const auto& destinationPath = m_installLocation / fileName;

            AICLI_LOG(CLI, Verbose, << "Getting Font title");

            std::wstring title = AppInstaller::Fonts::GetFontFileTitle(filePath);

            if (IsTrueTypeFont(fontFile.FileType))
            {
                title += s_TrueType;
            }

            // If font subkey already exists, remove the font file if it exists.
            if (m_key[title].has_value())
            {
                AICLI_LOG(CLI, Info, << "Existing font subkey found:" << AppInstaller::Utility::ConvertToUTF8(title));
                std::filesystem::path existingFontFilePath = { m_key[title]->GetValue<Registry::Value::Type::String>() };

                if (m_scope == Manifest::ScopeEnum::Machine)
                {
                    // Font entries in the HKEY_LOCAL_MACHINE hive only have the filename specified as the value. Prepend install location.
                    existingFontFilePath = m_installLocation / existingFontFilePath;
                }

                if (std::filesystem::exists(existingFontFilePath))
                {
                    AICLI_LOG(CLI, Info, << "Removing existing font file at:" << existingFontFilePath);
                    std::filesystem::remove(existingFontFilePath);
                }
            }

            AICLI_LOG(CLI, Info, << "Creating font subkey with name: " << AppInstaller::Utility::ConvertToUTF8(title));
            if (m_scope == Manifest::ScopeEnum::Machine)
            {
                m_key.SetValue(title, fileName, REG_SZ);
            }
            else
            {
                m_key.SetValue(title, destinationPath, REG_SZ);
            }

            AICLI_LOG(CLI, Info, << "Moving font file to: " << destinationPath);
            AppInstaller::Filesystem::RenameFile(filePath, destinationPath);
        }
    }

    void FontInstaller::Uninstall(const std::wstring& familyName)
    {
        UNREFERENCED_PARAMETER(familyName);
    }
}
