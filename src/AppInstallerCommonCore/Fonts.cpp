// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerStrings.h>
#include <winget/Fonts.h>
#include <winget/Locale.h>
#include <ShObjIdl_core.h>
#include <propkey.h>

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
                    path.resize(pathLength);
                    THROW_IF_FAILED(localLoader->GetFilePathFromKey(fontFileReferenceKey, fontFileReferenceKeySize, &path[0], pathLength));
                    path.resize(pathLength - 1); // Remove the null char.
                    filePaths.emplace_back(std::move(path));
                }
            }

            return filePaths;
        }
    }

    FontCatalog::FontCatalog()
    {
        m_preferredLocales = AppInstaller::Locale::GetUserPreferredLanguagesUTF16();
        THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(m_factory), m_factory.put_unknown()));
    }

    std::vector<FontFamily> FontCatalog::GetInstalledFontFamilies(std::optional<std::wstring> familyName)
    {
        wil::com_ptr<IDWriteFontCollection> collection;
        THROW_IF_FAILED(m_factory->GetSystemFontCollection(collection.addressof(), FALSE));

        std::vector<FontFamily> installedFontFamilies;

        if (familyName.has_value())
        {
            UINT32 index;
            BOOL exists;
            THROW_IF_FAILED(collection->FindFamilyName(familyName.value().c_str(), &index, &exists));

            if (exists)
            {
                installedFontFamilies.emplace_back(GetFontFamilyByIndex(collection, index));
            }
        }
        else
        {
            UINT32 familyCount = collection->GetFontFamilyCount();

            for (UINT32 index = 0; index < familyCount; index++)
            {
                installedFontFamilies.emplace_back(GetFontFamilyByIndex(collection, index));
            }
        }

        return installedFontFamilies;
    }

    bool FontCatalog::IsFontFileSupported(const std::filesystem::path& filePath, DWRITE_FONT_FILE_TYPE& fileType)
    {
        wil::com_ptr<IDWriteFontFile> fontFile;
        THROW_IF_FAILED(m_factory->CreateFontFileReference(filePath.c_str(), NULL, &fontFile));

        BOOL isSupported;
        DWRITE_FONT_FACE_TYPE faceType;
        UINT32 numOfFaces;
        THROW_IF_FAILED(fontFile->Analyze(&isSupported, &fileType, &faceType, &numOfFaces));
        return isSupported;
    }

    std::wstring FontCatalog::GetLocalizedStringFromFont(const wil::com_ptr<IDWriteLocalizedStrings>& localizedStringCollection)
    {
        UINT32 index = 0;
        BOOL exists = false;

        for (const auto& locale : m_preferredLocales)
        {
            if (SUCCEEDED_LOG(localizedStringCollection->FindLocaleName(locale.c_str(), &index, &exists)) && exists)
            {
                break;
            }
        }

        // If the locale does not exist, resort to the default value at the 0 index.
        if (!exists)
        {
            index = 0;
        }

        UINT32 length = 0;
        THROW_IF_FAILED(localizedStringCollection->GetStringLength(index, &length));
        length += 1; // Account for the trailing null terminator during allocation.

        std::wstring localizedString;
        localizedString.resize(length);
        THROW_IF_FAILED(localizedStringCollection->GetString(index, &localizedString[0], length));
        localizedString.resize(length - 1); // Remove the null char.
        return localizedString;
    }

    std::wstring FontCatalog::GetFontFaceName(const wil::com_ptr<IDWriteFont>& font)
    {
        wil::com_ptr<IDWriteLocalizedStrings> faceNames;
        THROW_IF_FAILED(font->GetFaceNames(faceNames.addressof()));
        return GetLocalizedStringFromFont(faceNames);
    }

    std::wstring FontCatalog::GetFontFamilyName(const wil::com_ptr<IDWriteFontFamily>& fontFamily)
    {
        wil::com_ptr<IDWriteLocalizedStrings> familyNames;
        THROW_IF_FAILED(fontFamily->GetFamilyNames(familyNames.addressof()));
        return GetLocalizedStringFromFont(familyNames);
    }

    Utility::OpenTypeFontVersion FontCatalog::GetFontFaceVersion(const wil::com_ptr<IDWriteFont>& font)
    {
        wil::com_ptr<IDWriteLocalizedStrings> fontVersion;
        BOOL exists;
        THROW_IF_FAILED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_VERSION_STRINGS, fontVersion.addressof(), &exists));
        if (!exists)
        {
            return {};
        }

        std::string value = AppInstaller::Utility::ConvertToUTF8(GetLocalizedStringFromFont(fontVersion));
        Utility::OpenTypeFontVersion openTypeFontVersion{ value };
        return openTypeFontVersion;
    }

    FontFamily FontCatalog::GetFontFamilyByIndex(const wil::com_ptr<IDWriteFontCollection>& collection, UINT32 index)
    {
        wil::com_ptr<IDWriteFontFamily> family;
        THROW_IF_FAILED(collection->GetFontFamily(index, family.addressof()));
        std::wstring familyName = GetFontFamilyName(family);

        std::vector<FontFace> fontFaces;
        UINT32 fontCount = family->GetFontCount();
        for (UINT32 fontIndex = 0; fontIndex < fontCount; fontIndex++)
        {
            wil::com_ptr<IDWriteFont> font;
            THROW_IF_FAILED(family->GetFont(fontIndex, font.addressof()));

            wil::com_ptr<IDWriteFontFace> fontFace;
            THROW_IF_FAILED(font->CreateFontFace(fontFace.addressof()));

            FontFace fontFaceEntry;
            fontFaceEntry.Name = GetFontFaceName(font);
            fontFaceEntry.Version = GetFontFaceVersion(font);
            fontFaceEntry.FilePaths = GetFontFilePaths(fontFace);
            fontFaces.emplace_back(std::move(fontFaceEntry));
        }

        FontFamily fontFamily;
        fontFamily.Name = std::move(familyName);
        fontFamily.Faces = std::move(fontFaces);
        return fontFamily;
    }

    std::wstring GetFontFileTitle(const std::filesystem::path& fontFilePath)
    {
        wil::com_ptr<IPropertyStore> pPropertyStore;
        THROW_IF_FAILED(SHGetPropertyStoreFromParsingName(fontFilePath.c_str(), nullptr, GPS_DEFAULT, IID_PPV_ARGS(&pPropertyStore)));
        PROPVARIANT prop;
        PropVariantInit(&prop);
        THROW_IF_FAILED(pPropertyStore->GetValue(PKEY_Title, &prop));
        std::wstring title = prop.pwszVal;
        THROW_IF_FAILED(PropVariantClear(&prop));
        return title;
    }
}
