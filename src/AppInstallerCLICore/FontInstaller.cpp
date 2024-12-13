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

    FontFile::FontFile(std::filesystem::path filePath, DWRITE_FONT_FILE_TYPE fileType)
        : FilePath(std::move(filePath)), FileType(fileType)
    {
        Title = AppInstaller::Fonts::GetFontFileTitle(FilePath);

        if (IsTrueTypeFont(FileType))
        {
            Title += s_TrueType;
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

    bool FontInstaller::EnsureInstall()
    {
        for (auto& fontFile : m_fontFiles)
        {
            if (m_key[fontFile.Title].has_value())
            {
                if (!std::filesystem::exists(m_key[fontFile.Title]->GetValue<Registry::Value::Type::String>()))
                {
                    AICLI_LOG(CLI, Info, << "Removing existing font value as font file does not exist.");
                    m_key.DeleteValue(fontFile.Title);
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Existing font value found: " << AppInstaller::Utility::ConvertToUTF8(fontFile.Title));
                    return false;
                }
            }

            std::filesystem::path destinationPath = m_installLocation / fontFile.FilePath.filename();
            auto initialStem = fontFile.FilePath.stem();
            auto extension = fontFile.FilePath.extension();

            // If a file exists at the destination path, make the filename unique.
            int index = 0;
            while (std::filesystem::exists(destinationPath))
            {
                std::filesystem::path unique = { "_" + std::to_string(index) };
                auto duplicateStem = initialStem;
                duplicateStem += unique;
                duplicateStem += extension;
                destinationPath = m_installLocation / duplicateStem;
                index++;
            }

            fontFile.DestinationPath = std::move(destinationPath);
        }

        return true;
    }

    void FontInstaller::Install()
    {
        bool isMachineScope = m_scope == Manifest::ScopeEnum::Machine;

        for (const auto& fontFile : m_fontFiles)
        {
            AICLI_LOG(CLI, Info, << "Creating font value with name : " << AppInstaller::Utility::ConvertToUTF8(fontFile.Title));
            if (isMachineScope)
            {
                m_key.SetValue(fontFile.Title, fontFile.DestinationPath.filename(), REG_SZ);
            }
            else
            {
                m_key.SetValue(fontFile.Title, fontFile.DestinationPath, REG_SZ);
            }
        }

        for (const auto& fontFile : m_fontFiles)
        {
            AICLI_LOG(CLI, Info, << "Moving font file to: " << fontFile.DestinationPath);
            AppInstaller::Filesystem::RenameFile(fontFile.FilePath, fontFile.DestinationPath);
        }
    }

    void FontInstaller::Uninstall()
    {
        for (const auto& fontFile : m_fontFiles)
        {
            if (m_key[fontFile.Title].has_value())
            {
                AICLI_LOG(CLI, Info, << "Existing font value found:" << AppInstaller::Utility::ConvertToUTF8(fontFile.Title));
                std::filesystem::path existingFontFilePath = { m_key[fontFile.Title]->GetValue<Registry::Value::Type::String>() };

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

                AICLI_LOG(CLI, Info, << "Deleting registry value:" << existingFontFilePath);
                m_key.DeleteValue(fontFile.Title);
            }
        }
    }
}
