// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerVersions.h>
#include <dwrite_3.h>
#include <wil/com.h>

namespace AppInstaller::Fonts
{
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

    struct FontCatalog
    {
        FontCatalog();

        // Gets all installed font families on the system. If an exact family name is provided and found, returns the installed font family.
        std::vector<FontFamily> GetInstalledFontFamilies(std::optional<std::wstring> familyName = {});

    private:
        FontFamily GetFontFamilyByIndex(const wil::com_ptr<IDWriteFontCollection>& collection, UINT32 index);
        std::wstring GetLocalizedStringFromFont(const wil::com_ptr<IDWriteLocalizedStrings>& localizedStringCollection);
        std::wstring GetFontFamilyName(const wil::com_ptr<IDWriteFontFamily>& fontFamily);
        std::wstring GetFontFaceName(const wil::com_ptr<IDWriteFont>& font);
        Utility::OpenTypeFontVersion GetFontFaceVersion(const wil::com_ptr<IDWriteFont>& font);

        std::vector<std::wstring> m_preferredLocales;
    };
}
