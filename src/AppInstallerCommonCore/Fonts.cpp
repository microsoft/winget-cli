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

            static_assert(sizeof(wil::com_ptr<IDWriteFontFile>) == sizeof(IDWriteFontFile*));
            std::vector<wil::com_ptr<IDWriteFontFile>> fontFiles;
            fontFiles.resize(fileCount);

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

                    std::wstring path;
                    path.reserve(pathLength);

                    THROW_IF_FAILED(localLoader->GetFilePathFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &path[0], pathLength));
                    std::filesystem::path fontFilePath = { std::move(path) };
                    filePaths.push_back(std::move(fontFilePath));
                }
            }

            return filePaths;
        }

        std::wstring GetLocalizedStringFromFont(const wil::com_ptr<IDWriteLocalizedStrings>& localizedStringCollection)
        {
            wchar_t localeNameBuffer[LOCALE_NAME_MAX_LENGTH];
            const auto localeName = GetUserDefaultLocaleName(localeNameBuffer, LOCALE_NAME_MAX_LENGTH) ? localeNameBuffer : L"en-US";

            UINT32 index;
            BOOL exists;
            if (FAILED(localizedStringCollection->FindLocaleName(localeName, &index, &exists)) || !exists)
            {
                index = 0;
            }

            UINT32 length = 0;
            THROW_IF_FAILED(localizedStringCollection->GetStringLength(index, &length));
            length += 1; // Account for the trailing null terminator during allocation.

            wchar_t* localizedString = new wchar_t[length];
            HRESULT hr = localizedStringCollection->GetString(index, localizedString, length);
            THROW_IF_FAILED(hr);
            return std::wstring(localizedString);
        }

        std::wstring GetFontFaceName(const wil::com_ptr<IDWriteFont>& font)
        {
            wil::com_ptr<IDWriteLocalizedStrings> faceNames;
            THROW_IF_FAILED(font->GetFaceNames(faceNames.addressof()));
            return GetLocalizedStringFromFont(faceNames);
        }

        std::wstring GetFontFamilyName(const wil::com_ptr<IDWriteFontFamily>& fontFamily)
        {
            wil::com_ptr<IDWriteLocalizedStrings> familyNames;
            THROW_IF_FAILED(fontFamily->GetFamilyNames(familyNames.addressof()));
            return GetLocalizedStringFromFont(familyNames);
        }

        wil::com_ptr<IDWriteFontFamily> CreateFontFamily(const wil::com_ptr<IDWriteFontCollection>& collection, UINT32 index)
        {
            wil::com_ptr<IDWriteFontFamily> family;
            THROW_IF_FAILED(collection->GetFontFamily(index, family.addressof()));
            return family;
        }
    }

    std::vector<FontFamily> GetInstalledFontFamilies()
    {
        wil::com_ptr<IDWriteFactory7> factory;
        THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(factory), factory.put_unknown()));

        wil::com_ptr<IDWriteFontCollection> collection;
        THROW_IF_FAILED(factory->GetSystemFontCollection(collection.addressof(), FALSE));

        UINT32 familyCount = collection->GetFontFamilyCount();

        std::vector<FontFamily> fontFamilies;
        for (UINT32 index = 0; index < familyCount; index++)
        {
            wil::com_ptr<IDWriteFontFamily> family = CreateFontFamily(collection, index);
            std::wstring familyName = GetFontFamilyName(family);

            std::vector<FontFace> fontFaces;
            UINT32 fontCount = family->GetFontCount();
            for (UINT32 fontIndex = 0; fontIndex < fontCount; fontIndex++)
            {
                wil::com_ptr<IDWriteFont> font;
                THROW_IF_FAILED(family->GetFont(fontIndex, font.addressof()));

                std::wstring faceName = GetFontFaceName(font);

                wil::com_ptr<IDWriteFontFace> fontFace;
                THROW_IF_FAILED(font->CreateFontFace(fontFace.addressof()));

                FontFace fontFaceEntry;
                fontFaceEntry.Name = faceName;
                fontFaceEntry.FilePaths = GetFontFilePaths(fontFace);
                fontFaces.emplace_back(std::move(fontFaceEntry));
            }

            FontFamily fontFamily;
            fontFamily.Name = std::move(familyName);
            fontFamily.Faces = std::move(fontFaces);
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

        UINT32 index;
        BOOL exists;
        THROW_IF_FAILED(collection->FindFamilyName(familyName.c_str(), &index, &exists));

        if (!exists)
        {
            return {};
        }

        wil::com_ptr<IDWriteFontFamily> family = CreateFontFamily(collection, index);
        UINT32 fontCount = family->GetFontCount();

        std::vector<FontFace> fontFaces;
        for (UINT32 fontIndex = 0; fontIndex < fontCount; fontIndex++)
        {
            wil::com_ptr<IDWriteFont> font;
            THROW_IF_FAILED(family->GetFont(fontIndex, font.addressof()));
            std::wstring faceName = GetFontFaceName(font);

            wil::com_ptr<IDWriteFontFace> fontFace;
            THROW_IF_FAILED(font->CreateFontFace(fontFace.addressof()));

            FontFace fontFaceEntry;
            fontFaceEntry.Name = faceName;
            fontFaceEntry.FilePaths = GetFontFilePaths(fontFace);
            fontFaces.emplace_back(std::move(fontFaceEntry));
        }

        FontFamily fontFamily;
        fontFamily.Name = GetFontFamilyName(family);
        fontFamily.Faces = std::move(fontFaces);
        return fontFamily;
    }
}
