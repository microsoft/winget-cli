// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Fonts.h"
#include <dwrite_3.h>

namespace AppInstaller::Fonts
{
    std::vector<std::wstring> GetInstalledFontFamilies()
    {
        wchar_t localeNameBuffer[LOCALE_NAME_MAX_LENGTH];
        const auto localeName = GetUserDefaultLocaleName(localeNameBuffer, LOCALE_NAME_MAX_LENGTH) ? localeNameBuffer : L"en-US";

        wil::com_ptr<IDWriteFactory7> factory;
        THROW_IF_FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(factory), factory.put_unknown()));

        wil::com_ptr<IDWriteFontCollection> collection;
        THROW_IF_FAILED(factory->GetSystemFontCollection(collection.addressof(), FALSE));

        const auto familyCount = collection->GetFontFamilyCount();
        std::vector<std::wstring> familyNames;

        for (UINT32 index = 0; index < familyCount; index++)
        {
            wil::com_ptr<IDWriteFontFamily> family;
            THROW_IF_FAILED(collection->GetFontFamily(index, family.addressof()));

            wil::com_ptr<IDWriteFont> singleFont;
            THROW_IF_FAILED(family->GetFont(0, &singleFont));

            // Prepare to retrieve font information
            wil::com_ptr<IDWriteLocalizedStrings> fullName;
            BOOL fullNameExists;
            THROW_IF_FAILED(singleFont->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_FULL_NAME, &fullName, &fullNameExists));
            UINT32 fullNameIndex;
            if (FAILED(fullName->FindLocaleName(localeName, &fullNameIndex, &fullNameExists)) || !fullNameExists)
            {
                fullNameIndex = 0;
            }

            UINT32 fullNameLength = 0;
            THROW_IF_FAILED(fullName->GetStringLength(fullNameIndex, &fullNameLength));
            fullNameLength += 1; // Account for the trailing null terminator during allocation.

            wchar_t result[512];
            THROW_HR_IF(E_OUTOFMEMORY, fullNameLength > ARRAYSIZE(result));
            THROW_IF_FAILED(fullName->GetString(fullNameIndex, &result[0], fullNameLength));
            std::wstring familyName(result);
            familyNames.emplace_back(familyName);
        }

        return familyNames;
    }
}
