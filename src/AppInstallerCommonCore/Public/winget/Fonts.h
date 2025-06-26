// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <dwrite_3.h>
#include <wil/com.h>
#include <winget/Manifest.h>
#include <winget/ManifestCommon.h>

namespace AppInstaller::Fonts
{
    enum class InstallerSource
    {
        // Installer source is not guaranteed to be absolutely correct, as it is just
        // filesystem and registry. We make assumptions based on the structure of how
        // the font is installed to make a best-effort determination.
        Unknown,    // Can be any external font installer
        UWP,        // Font installed by a Universal Windows Platform app
        WinGet,     // Font installed by WinGet
        Any,        // Can be any installer.
    };

    enum class FontStatus
    {
        Unknown,
        Missing,    // Font file is missing from it's expected location.
        OK,         // Font is in a good state.
    };

    enum class FontResult
    {
        Unknown,
        Cancelled,
        Success,
        Error,
    };

    struct FontFace
    {
        std::wstring Name;
        std::vector<std::filesystem::path> FilePaths;
        Utility::OpenTypeFontVersion Version;
    };

    struct FontFamily
    {
        std::wstring Name;
        std::vector<FontFace> Faces;
    };


    // Represents information about a font file used for its installation, query, and removal.
    struct FontFileInfo
    {
        std::filesystem::path FilePath;
        std::wstring Title;
        Manifest::ScopeEnum Scope = Manifest::ScopeEnum::Unknown;
        DWRITE_FONT_FILE_TYPE FileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
        InstallerSource InstallerSource = InstallerSource::Unknown;
        FontStatus Status = FontStatus::Unknown;
        bool WinGetSupported = false;

        // A file may be present and not properly linked to a registry location
        std::optional<std::wstring> RegistryPath;

        // For Fonts installed with WinGet
        std::optional<std::wstring> PackageName;
        std::optional<std::wstring> PackageFullName;
        std::optional<Utility::Version> PackageVersion;
    };

    struct FontContext
    {
        InstallerSource InstallerSource = InstallerSource::Unknown;
        Manifest::ScopeEnum Scope = Manifest::ScopeEnum::Unknown;
        std::optional<std::filesystem::path> FilePath;
        std::optional<std::wstring> PackageName;
        std::optional<std::wstring> Title;
    };

    struct FontOperationResult
    {
        FontResult Result = FontResult::Unknown;
        winrt::hresult HResult = winrt::hresult(S_OK);

        // TODO: Add optional rollback context to unwind forced installs.
    };

    std::wstring GetFontRegistryPath(const FontContext& context);

    std::filesystem::path GetFontFileInstallPath(const FontContext& context);

    std::wstring GetFontFileTitle(const std::filesystem::path& fontFilePath);

    std::vector<FontFileInfo> GetInstalledFontFiles();

    FontFileInfo CreateFontFileInfo(const FontContext& context);

    FontOperationResult InstallFontFile(FontContext& context, const bool notifySystem = false, const bool force = false);

    FontOperationResult UninstallFontFile(FontContext& context, const bool notifySystem = false);

    struct FontCatalog
    {
        FontCatalog();

        // Gets all installed font families on the system. If an exact family name is provided and found, returns the installed font family.
        std::vector<FontFamily> GetInstalledFontFamilies(std::optional<std::wstring> familyName = {});

        // Returns a boolean value indicating whether the specified file path is a valid font file.
        bool IsFontFileSupported(const std::filesystem::path& filePath, DWRITE_FONT_FILE_TYPE& fileType);

        std::filesystem::path GetRootFontPath(Manifest::ScopeEnum scope);

    private:
        FontFamily GetFontFamilyByIndex(const wil::com_ptr<IDWriteFontCollection>& collection, UINT32 index);
        std::wstring GetLocalizedStringFromFont(const wil::com_ptr<IDWriteLocalizedStrings>& localizedStringCollection);
        std::wstring GetFontFamilyName(const wil::com_ptr<IDWriteFontFamily>& fontFamily);
        std::wstring GetFontFaceName(const wil::com_ptr<IDWriteFont>& font);
        Utility::OpenTypeFontVersion GetFontFaceVersion(const wil::com_ptr<IDWriteFont>& font);

        wil::com_ptr<IDWriteFactory7> m_factory;
        std::vector<std::wstring> m_preferredLocales;
    };
}
