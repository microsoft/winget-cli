// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <dwrite_3.h>
#include <AppInstallerStrings.h>
#include <winget/Fonts.h>

namespace AppInstaller::Fonts
{
    namespace
    {
        std::vector<std::filesystem::path> GetFontFilePaths(const wil::com_ptr<IDWriteFontFace>& fontFace)
        {
            UINT32 fileCount = 0;
            THROW_IF_FAILED(fontFace->GetFiles(&fileCount, nullptr));

            wil::com_ptr<IDWriteFontFile> fontFiles[8];
            THROW_HR_IF(E_OUTOFMEMORY, fileCount > ARRAYSIZE(fontFiles));
            THROW_IF_FAILED(fontFace->GetFiles(&fileCount, fontFiles[0].addressof()));

            std::vector<std::filesystem::path> filePaths;
            for (UINT32 i = 0; i < fileCount; ++i) {
                wil::com_ptr<IDWriteFontFileLoader> loader;
                THROW_IF_FAILED(fontFiles[i]->GetLoader(loader.addressof()));

                const void* fontFileReferenceKey;
                UINT32 fontFileReferenceKeySize;
                THROW_IF_FAILED(fontFiles[i]->GetReferenceKey(&fontFileReferenceKey, &fontFileReferenceKeySize));

                if (const auto localLoader = loader.try_query<IDWriteLocalFontFileLoader>()) {
                    UINT32 pathLength;
                    THROW_IF_FAILED(localLoader->GetFilePathLengthFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &pathLength));
                    pathLength += 1; // Account for the trailing null terminator during allocation.

                    wchar_t path[MAX_PATH];
                    THROW_HR_IF(E_OUTOFMEMORY, pathLength > ARRAYSIZE(path));
                    THROW_IF_FAILED(localLoader->GetFilePathFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &path[0], pathLength));
                    std::filesystem::path fontFilePath = { AppInstaller::Utility::Normalize(std::wstring(path)) };
                    filePaths.push_back(std::move(fontFilePath));
                }
            }

            return std::move(filePaths);
        }

        std::wstring GetFontFaceName(const wil::com_ptr<IDWriteFont>& font)
        {
            wil::com_ptr<IDWriteLocalizedStrings> faceNames;
            THROW_IF_FAILED(font->GetFaceNames(faceNames.addressof()));

            wchar_t localeNameBuffer[LOCALE_NAME_MAX_LENGTH];
            const auto localeName = GetUserDefaultLocaleName(localeNameBuffer, LOCALE_NAME_MAX_LENGTH) ? localeNameBuffer : L"en-US";

            UINT32 faceNameIndex;
            BOOL faceNameExists;
            if (FAILED(faceNames->FindLocaleName(localeName, &faceNameIndex, &faceNameExists)) || !faceNameExists)
            {
                faceNameIndex = 0;
            }

            UINT32 faceNameLength = 0;
            THROW_IF_FAILED(faceNames->GetStringLength(faceNameIndex, &faceNameLength));
            faceNameLength += 1; // Account for the trailing null terminator during allocation.

            wchar_t faceName[512];
            THROW_HR_IF(E_OUTOFMEMORY, faceNameLength > ARRAYSIZE(faceName));
            THROW_IF_FAILED(faceNames->GetString(faceNameIndex, &faceName[0], faceNameLength));
            return std::wstring(faceName);
        }

        std::wstring GetFontFamilyName(const wil::com_ptr<IDWriteFontFamily>& fontFamily)
        {
            // Retrieve family names.
            wil::com_ptr<IDWriteLocalizedStrings> familyNames;
            THROW_IF_FAILED(fontFamily->GetFamilyNames(familyNames.addressof()));

            wchar_t localeNameBuffer[LOCALE_NAME_MAX_LENGTH];
            const auto localeName = GetUserDefaultLocaleName(localeNameBuffer, LOCALE_NAME_MAX_LENGTH) ? localeNameBuffer : L"en-US";

            UINT32 familyNameIndex;
            BOOL familyNameExists;
            if (FAILED(familyNames->FindLocaleName(localeName, &familyNameIndex, &familyNameExists)) || !familyNameExists)
            {
                familyNameIndex = 0;
            }

            UINT32 familyNameLength = 0;
            THROW_IF_FAILED(familyNames->GetStringLength(familyNameIndex, &familyNameLength));
            familyNameLength += 1; // Account for the trailing null terminator during allocation.

            wchar_t familyName[512];
            THROW_HR_IF(E_OUTOFMEMORY, familyNameLength > ARRAYSIZE(familyName));
            THROW_IF_FAILED(familyNames->GetString(0, &familyName[0], familyNameLength));
            return std::wstring(familyName);
        }
    }

    std::vector<FontFamily> GetInstalledFontFamilies()
    {
        wil::com_ptr<IDWriteFactory7> factory;
        THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(factory), factory.put_unknown()));

        wil::com_ptr<IDWriteFontCollection> collection;
        THROW_IF_FAILED(factory->GetSystemFontCollection(collection.addressof(), FALSE));

        const auto familyCount = collection->GetFontFamilyCount();

        std::vector<FontFamily> fontFamilies;
        for (UINT32 index = 0; index < familyCount; index++)
        {
            wil::com_ptr<IDWriteFontFamily> family;
            THROW_IF_FAILED(collection->GetFontFamily(index, family.addressof()));

            std::wstring familyName = GetFontFamilyName(family);

            std::vector<FontFace> fontFaces;
            const auto& fontCount = family->GetFontCount();
            for (UINT32 fontIndex = 0; fontIndex < fontCount; fontIndex++)
            {
                wil::com_ptr<IDWriteFont> font;
                THROW_IF_FAILED(family->GetFont(fontIndex, font.addressof()));
                std::wstring faceName = GetFontFaceName(font);

                wil::com_ptr<IDWriteFontFace> fontFace;
                THROW_IF_FAILED(font->CreateFontFace(fontFace.addressof()));

                const auto& filePaths = GetFontFilePaths(fontFace);

                FontFace fontFaceEntry;
                fontFaceEntry.Name = std::wstring(faceName);
                fontFaceEntry.FilePaths = filePaths;
                fontFaces.emplace_back(fontFaceEntry);
            }

            FontFamily fontFamily;
            fontFamily.Name = familyName;
            fontFamily.Faces = fontFaces;
            fontFamilies.emplace_back(std::move(fontFamily));
        }

        return fontFamilies;
    }

    std::optional<FontFamily> GetInstalledFontFamily(const std::wstring& familyName)
    {
        wil::com_ptr<IDWriteFactory7> factory;
        THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(factory), factory.put_unknown()));

        wil::com_ptr<IDWriteFontCollection> collection;
        THROW_IF_FAILED(factory->GetSystemFontCollection(collection.addressof(), FALSE));

        UINT32 familyIndex;
        BOOL familyExists;
        THROW_IF_FAILED(collection->FindFamilyName(familyName.c_str(), &familyIndex, &familyExists));

        if (!familyExists)
        {
            return {};
        }

        wil::com_ptr<IDWriteFontFamily> family;
        THROW_IF_FAILED(collection->GetFontFamily(familyIndex, family.addressof()));
        const auto fontCount = family->GetFontCount();

        std::vector<FontFace> fontFaces;
        for (UINT32 fontIndex = 0; fontIndex < fontCount; fontIndex++)
        {
            wil::com_ptr<IDWriteFont> font;
            THROW_IF_FAILED(family->GetFont(fontIndex, font.addressof()));
            std::wstring faceName = GetFontFaceName(font);

            wil::com_ptr<IDWriteFontFace> fontFace;
            THROW_IF_FAILED(font->CreateFontFace(fontFace.addressof()));

            const auto& filePaths = GetFontFilePaths(fontFace);

            FontFace fontFaceEntry;
            fontFaceEntry.Name = std::wstring(faceName);
            fontFaceEntry.FilePaths = filePaths;
            fontFaces.emplace_back(fontFaceEntry);
        }

        FontFamily fontFamily;
        fontFamily.Name = familyName;
        fontFamily.Faces = std::move(fontFaces);
        return fontFamily;
    }
}
