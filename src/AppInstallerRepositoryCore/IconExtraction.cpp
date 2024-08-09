// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "IconDefs.h"
#include "winget/IconExtraction.h"
#include "Microsoft/ARPHelper.h"
#include <AppInstallerSHA256.h>
#include <winget/Filesystem.h>

namespace AppInstaller::Repository
{
    using namespace AppInstaller::Repository::Microsoft;

    namespace
    {
        // Struct used as data object passed to Enumerate callback function of EnumResourceNamesEx
        struct EnumGroupIconProcParameter
        {
            // Input to specify icon index
            int IconIndex = 0;
            // The result Resource handle of the group icon
            HRSRC ResourceHandle = nullptr;
            // How many icons were already found
            int IconsFound = 0;
        };

        BOOL CALLBACK EnumGroupIconProc(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam)
        {
            EnumGroupIconProcParameter* parameter = reinterpret_cast<EnumGroupIconProcParameter*>(lParam);
            bool foundRequestedIcon = false;

            // Find icon by resource name
            if (parameter->IconIndex < 0)
            {
                if (IS_INTRESOURCE(lpName))
                {
                    if (-parameter->IconIndex == LOWORD(lpName))
                    {
                        // Found icon by MAKEINTRESOURCE name
                        foundRequestedIcon = true;
                    }
                }
                else if (lpName[0] == TEXT('#'))
                {
                    std::wstring resourceIdString = lpName + 1; // skip the #
                    try
                    {
                        auto resourceId = std::stoi(resourceIdString.c_str(), nullptr, 0);
                        if (-parameter->IconIndex == resourceId)
                        {
                            // Found icon by number as string #12
                            foundRequestedIcon = true;
                        }
                    }
                    catch (...)
                    {
                        // Error occurred, stop enumerating
                        return FALSE;
                    }
                }
            }
            else if (parameter->IconIndex == parameter->IconsFound)
            {
                // Found icon by index
                foundRequestedIcon = TRUE;
            }

            if (foundRequestedIcon)
            {
                parameter->ResourceHandle = FindResourceExW(hModule, lpType, lpName, 0);
                return FALSE;
            }

            // Continue enumerating
            parameter->IconsFound++;
            return TRUE;
        };

        void WriteIconDirHeaderToByteArray(std::vector<BYTE>& data, const ICONDIR& iconDir)
        {
            data.clear();
            BYTE const* toBeWritten = reinterpret_cast<BYTE const*>(&(iconDir.idReserved));
            data.insert(data.end(), toBeWritten, toBeWritten + sizeof(iconDir.idReserved));
            toBeWritten = reinterpret_cast<BYTE const*>(&(iconDir.idType));
            data.insert(data.end(), toBeWritten, toBeWritten + sizeof(iconDir.idType));
            toBeWritten = reinterpret_cast<BYTE const*>(&(iconDir.idCount));
            data.insert(data.end(), toBeWritten, toBeWritten + sizeof(iconDir.idCount));
        }

        void AppendIconDirEntryToByteArray(std::vector<BYTE>& data, const ICONDIRENTRY& iconDirEntry)
        {
            data.insert(data.end(), iconDirEntry.bWidth);
            data.insert(data.end(), iconDirEntry.bHeight);
            data.insert(data.end(), iconDirEntry.bColorCount);
            data.insert(data.end(), iconDirEntry.bReserved);
            BYTE const* toBeWritten = reinterpret_cast<BYTE const*>(&(iconDirEntry.wPlanes));
            data.insert(data.end(), toBeWritten, toBeWritten + sizeof(iconDirEntry.wPlanes));
            toBeWritten = reinterpret_cast<BYTE const*>(&(iconDirEntry.wBitCount));
            data.insert(data.end(), toBeWritten, toBeWritten + sizeof(iconDirEntry.wBitCount));
            toBeWritten = reinterpret_cast<BYTE const*>(&(iconDirEntry.dwBytesInRes));
            data.insert(data.end(), toBeWritten, toBeWritten + sizeof(iconDirEntry.dwBytesInRes));
            toBeWritten = reinterpret_cast<BYTE const*>(&(iconDirEntry.dwImageOffset));
            data.insert(data.end(), toBeWritten, toBeWritten + sizeof(iconDirEntry.dwImageOffset));
        }
    }

    std::vector<BYTE> ExtractIconFromBinaryFile(const std::filesystem::path binaryPath, int iconIndex)
    {
        try
        {
            wil::unique_hmodule module;
            module.reset(LoadLibraryEx(binaryPath.c_str(), nullptr, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE));
            THROW_LAST_ERROR_IF_NULL(module);

            EnumGroupIconProcParameter param;
            param.IconIndex = iconIndex;

#pragma warning( push )
#pragma warning ( disable : 4302 )
            // First find the requested group icon
            EnumResourceNamesExW(
                module.get(),
                MAKEINTRESOURCE(RT_GROUP_ICON),
                EnumGroupIconProc,
                reinterpret_cast<LONG_PTR>(&param),
                (RESOURCE_ENUM_MUI | RESOURCE_ENUM_LN | RESOURCE_ENUM_VALIDATE),
                0);
#pragma warning( pop )

            if (param.ResourceHandle)
            {
                // Load and Lock to get a pointer to a GRPICONDIR
                HGLOBAL groupIconResourceHandle = LoadResource(module.get(), param.ResourceHandle);
                THROW_LAST_ERROR_IF_NULL(groupIconResourceHandle);
                LPGRPICONDIR groupIconDir = reinterpret_cast<LPGRPICONDIR>(LockResource(groupIconResourceHandle));
                THROW_LAST_ERROR_IF_NULL(groupIconDir);

                // Basic validation
                if (groupIconDir->idReserved != 0 || groupIconDir->idType != 1 || groupIconDir->idCount == 0)
                {
                    return {};
                }

                struct SingleIconImage
                {
                    ICONDIRENTRY DirEntry = { 0 };
                    // pointer to byte contents with size
                    std::pair<const BYTE*, DWORD> Content;
                };

                // Read all individual icon image contents
                std::vector<SingleIconImage> iconContents;
                // The first image's offset.
                DWORD imageOffset = 6 /* ICONDIR size */ + groupIconDir->idCount * 16 /* each ICONDIRENTRY size */;

                for (int i = 0; i < groupIconDir->idCount; i++)
                {
                    SingleIconImage iconEntry;

                    // Populate ICONDIRENTRY
                    iconEntry.DirEntry.bWidth = groupIconDir->idEntries[i].bWidth;
                    iconEntry.DirEntry.bHeight = groupIconDir->idEntries[i].bHeight;
                    iconEntry.DirEntry.bColorCount = groupIconDir->idEntries[i].bColorCount;
                    iconEntry.DirEntry.bReserved = groupIconDir->idEntries[i].bReserved;
                    iconEntry.DirEntry.wPlanes = groupIconDir->idEntries[i].wPlanes;
                    iconEntry.DirEntry.wBitCount = groupIconDir->idEntries[i].wBitCount;
                    iconEntry.DirEntry.dwBytesInRes = groupIconDir->idEntries[i].dwBytesInRes;
                    iconEntry.DirEntry.dwImageOffset = imageOffset;

                    // Load individual icon content
                    HRSRC iconResourceHandle = FindResourceExW(module.get(), RT_ICON, MAKEINTRESOURCE(groupIconDir->idEntries[i].nID), 0);
                    THROW_LAST_ERROR_IF_NULL(iconResourceHandle);
                    HGLOBAL iconResourceContentHandle = LoadResource(module.get(), iconResourceHandle);
                    THROW_LAST_ERROR_IF_NULL(iconResourceContentHandle);
                    iconEntry.Content.second = SizeofResource(module.get(), iconResourceHandle);
                    THROW_LAST_ERROR_IF(iconEntry.Content.second == 0);
                    iconEntry.Content.first = reinterpret_cast<BYTE*>(LockResource(iconResourceContentHandle));
                    THROW_LAST_ERROR_IF_NULL(iconEntry.Content.first);

                    // This will be the next image offset.
                    imageOffset += iconEntry.Content.second;

                    iconContents.emplace_back(std::move(iconEntry));
                }

                // Construct ico file icon dir header
                ICONDIR iconDir;
                iconDir.idReserved = groupIconDir->idReserved;
                iconDir.idType = groupIconDir->idType;
                iconDir.idCount = groupIconDir->idCount;

                std::vector<BYTE> result;

                // Write Icon Dir header
                WriteIconDirHeaderToByteArray(result, iconDir);

                // Write Icon Dir entries
                for (auto const& singleIconEntry : iconContents)
                {
                    AppendIconDirEntryToByteArray(result, singleIconEntry.DirEntry);
                }

                // Write Icon contents
                for (auto const& singleIconEntry : iconContents)
                {
                    result.insert(result.end(), singleIconEntry.Content.first, singleIconEntry.Content.first + singleIconEntry.Content.second);
                }

                return result;
            }
        }
        CATCH_LOG();

        return {};
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static std::vector<ExtractedIconInfo>* s_ExtractIconFromArpEntry_TestHook_Override = nullptr;

    void TestHook_SetExtractIconFromArpEntryResult_Override(std::vector<ExtractedIconInfo>* result)
    {
        s_ExtractIconFromArpEntry_TestHook_Override = result;
    }
#endif

    std::vector<ExtractedIconInfo> ExtractIconFromArpEntry(const std::string& productCode, Manifest::ScopeEnum scope)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_ExtractIconFromArpEntry_TestHook_Override)
        {
            return *s_ExtractIconFromArpEntry_TestHook_Override;
        }
#endif

        ARPHelper arpHelper;
        Registry::Key arpEntry = arpHelper.FindARPEntry(productCode, scope);

        if (arpEntry)
        {
            std::wstring iconPathRaw;
            if (arpHelper.GetBoolValue(arpEntry, arpHelper.WindowsInstaller))
            {
                // For msi, get icon from ProductInfo
                auto productCodeWide = Utility::ConvertToUTF16(productCode);
                DWORD iconPathSize = 0;
                if (ERROR_MORE_DATA == MsiGetProductInfoW(productCodeWide.c_str(), INSTALLPROPERTY_PRODUCTICON, nullptr, &iconPathSize))
                {
                    std::wstring iconPathBuffer;
                    // The iconPathSize returned in previous call does not count the null terminator.
                    iconPathSize++;
                    iconPathBuffer.resize(iconPathSize);
                    if (ERROR_SUCCESS == MsiGetProductInfoW(productCodeWide.c_str(), INSTALLPROPERTY_PRODUCTICON, iconPathBuffer.data(), &iconPathSize))
                    {
                        iconPathBuffer.resize(iconPathSize);
                        iconPathRaw = iconPathBuffer;
                    }
                }
            }
            else
            {
                // For other win32 apps, try DisplayIcon.
                iconPathRaw = Utility::ConvertToUTF16(arpHelper.GetStringValue(arpEntry, arpHelper.DisplayIcon));
            }

            if (!iconPathRaw.empty())
            {
                PathUnquoteSpacesW(iconPathRaw.data());
                // For paths like C:\test\test.exe,-3
                int iconIndex = 0;
                iconIndex = PathParseIconLocationW(iconPathRaw.data());
                // Above operations will modify the input string with null terminator in the middle.
                iconPathRaw = iconPathRaw.c_str();
                auto iconPath = Filesystem::GetExpandedPath(Utility::ConvertToUTF8(iconPathRaw));

                if (std::filesystem::exists(iconPath))
                {
                    auto extension = iconPath.extension().u8string();
                    std::vector<BYTE> iconContent;
                    if (Utility::CaseInsensitiveEquals(extension, ".ico"))
                    {
                        std::ifstream iconFile{ iconPath, std::ios::in | std::ios::binary };
                        iconContent = Utility::ReadEntireStreamAsByteArray(iconFile);
                    }
                    else if (Utility::CaseInsensitiveEquals(extension, ".exe") || Utility::CaseInsensitiveEquals(extension, ".dll"))
                    {
                        iconContent = ExtractIconFromBinaryFile(iconPath, iconIndex);
                    }

                    // Construct ExtractedIconInfo return result
                    if (!iconContent.empty())
                    {
                        ExtractedIconInfo iconInfo;
                        iconInfo.IconFileType = Manifest::IconFileTypeEnum::Ico;
                        iconInfo.IconTheme = Manifest::IconThemeEnum::Default;
                        iconInfo.IconResolution = Manifest::IconResolutionEnum::Custom;
                        iconInfo.IconSha256 = Utility::SHA256::ComputeHash(iconContent.data(), static_cast<uint32_t>(iconContent.size()));
                        iconInfo.IconContent = std::move(iconContent);

                        return { std::move(iconInfo) };
                    }
                }
            }
        }

        return {};
    }
}