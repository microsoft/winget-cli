// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <winget/Filesystem.h>
#include <winget/Fonts.h>
#include <winget/Locale.h>
#include <winget/Manifest.h>
#include <winget/ManifestCommon.h>
#include <winget/Registry.h>
#include <ShObjIdl_core.h>
#include <propkey.h>
#include <wingdi.h>

using namespace AppInstaller::Utility;

namespace AppInstaller::Fonts
{
    constexpr std::wstring_view s_FontsPathSubkey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
    constexpr std::wstring_view s_FontsUserInstallFolder = L"Microsoft\\Windows\\Fonts";
    constexpr std::wstring_view s_TrueType = L" (TrueType)";

    namespace
    {
        constexpr std::wstring_view s_RegistrySeparator = L"\\";
        constexpr std::wstring_view s_FontsWinGetPrefix = L"winget_v1";

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

        winrt::hresult NotifyFontChange()
        {
            // Send the WM_FONTCHANGE message so the system and apps know that there has been a font change.
            RETURN_LAST_ERROR_IF(0 <= SendMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0));
            AICLI_LOG(Core, Info, << L"Notified system of font change.");
            return S_OK;
        }

        // Rollback is best-effort and does not throw, as it is expected to be called in a catch block.
        // Normal rollback of a WinGet install is:
        // 1) Remove any font references for parts of the install that were successful (any registry keys already set)
        // 2) Move the folder with the fonts and delete the folder (or mark it for deletion next restart).
        // 3) Send update that fonts have changed.
        bool RollbackFontInstalls(/* Rollback Context*/) try
        {
            bool success = false;

            // Report to caller whether rollback was successful without error.
            return success;
        }
        CATCH_LOG()
    }

    std::wstring GetFontRegistryPath(const FontContext& context)
    {
        // Registry path for a font is well-defined based on installer source, and if present a package name.
        auto path = std::wstringstream();

        path << s_FontsPathSubkey;

        switch (context.InstallerSource)
        {
        case InstallerSource::Unknown:
            // Unknown installer is assumed to be installed in the default locations.
            break;
        case InstallerSource::UWP:
            // UWP path adds the package full name
            if (context.PackageName.has_value() && !context.PackageName.value().empty())
            {
                path << s_RegistrySeparator << context.PackageName.value();
            }
            break;
        case InstallerSource::WinGet:
            // WinGet path adds the WinGet prefix and the package name
            if (context.PackageName.has_value() && !context.PackageName.value().empty())
            {
                path << s_RegistrySeparator << s_FontsWinGetPrefix << s_RegistrySeparator << context.PackageName.value();
            }
            else
            {
                throw std::runtime_error("WinGet Font paths require a package name.");
            }
            break;
        default:
            throw std::runtime_error("Unexpected InstallerSource in GetFontRegistryPath");
        }

        return path.str();
    }

    std::filesystem::path GetFontFileInstallPath(const FontContext& context)
    {
        auto path = std::filesystem::path();

        switch (context.Scope)
        {
        case Manifest::ScopeEnum::Machine:
            path /= Runtime::GetPathTo(Runtime::PathName::FontsMachineInstallLocation);
            break;
        case Manifest::ScopeEnum::User:
            path /= Runtime::GetPathTo(Runtime::PathName::FontsUserInstallLocation);
            break;
        default:
            throw std::runtime_error("Unexpected scope in GetFontFileInstallPath");
        }

        switch (context.InstallerSource)
        {
        case InstallerSource::Unknown:
            // Unknown installer is assumed to be installed in the default locations.
            break;
        case InstallerSource::UWP:
            // UWP path could be anywhere, but is probably in the single instanced store.
            // Since we have no intended support to mess with UWP fonts, we will assume it
            // is in the default location.
            break;
        case InstallerSource::WinGet:
            // WinGet path adds the WinGet prefix and the package name
            if (context.PackageName.has_value() && !context.PackageName.value().empty())
            {
                path /= s_FontsWinGetPrefix;
                path /= context.PackageName.value();
            }
            else
            {
                throw std::runtime_error("WinGet Font paths require a package name.");
            }
            break;
        default:
            throw std::runtime_error("Unexpected InstallerSource in GetFontRegistryPath");
        }

        return path;
    }

    // Creates font file info given a path, title, package name, scope and installer source.
    FontFileInfo CreateFontFileInfo(const FontContext& context)
    {
        auto fontFile = FontFileInfo();
        fontFile.Title = context.Title.value_or(L"");
        fontFile.FilePath = context.FilePath.value_or(L"");
        fontFile.PackageFullName = context.PackageName.value_or(L"");
        fontFile.InstallerSource = context.InstallerSource;
        fontFile.Scope = context.Scope;

        auto fontCatalog = FontCatalog();

        switch (fontFile.InstallerSource)
        {
        case InstallerSource::UWP:
            {
                const auto& sepPosition = fontFile.PackageFullName.value_or(L"").find('_');
                fontFile.PackageName = fontFile.PackageFullName.value_or(L"").substr(0, sepPosition);
            }
            break;

        case InstallerSource::WinGet:
            break;

        case InstallerSource::Unknown:
            break;
        }

        fontFile.RegistryPath = GetFontRegistryPath(context);

        if (fontFile.FilePath.is_relative())
        {
            // Relative path is assumed to be relative to the default location for the scope.
            auto fullPath = fontCatalog.GetRootFontPath(fontFile.Scope);
            fullPath /= fontFile.FilePath;
            fontFile.FilePath = std::move(fullPath);
        }

        // The registry could refer to a file that is not present.
        if (std::filesystem::exists(fontFile.FilePath))
        {
            fontFile.WinGetSupported = fontCatalog.IsFontFileSupported(fontFile.FilePath, fontFile.FileType);
            fontFile.Status = FontStatus::OK;
        }
        else
        {
            fontFile.WinGetSupported = false;
            fontFile.FileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
            fontFile.Status = FontStatus::Missing;
        }

        return fontFile;
    }

    FontOperationResult InstallFontFile(FontContext& context, const bool notifySystem, const bool force)
    {
        DBG_UNREFERENCED_PARAMETER(notifySystem);
        DBG_UNREFERENCED_PARAMETER(force);

        FontOperationResult result;

        if (context.InstallerSource != InstallerSource::WinGet)
        {
            // THROW_EXCEPTION(E_NOTIMPL);
        }

        if (!context.FilePath.has_value() || !std::filesystem::exists(context.FilePath.value()))
        {
            result.Result = FontResult::Error;
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_FOUND);
            AICLI_LOG(Core, Error, << L"Font file was not found: " << context.FilePath.value_or(L"") << " - " << APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_FOUND);
            return result;
        }

        AICLI_LOG(Core, Info, << "Starting install of " << context.FilePath.value());

        auto fontCatalog = FontCatalog();
        DWRITE_FONT_FILE_TYPE fileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
        const auto& supported = fontCatalog.IsFontFileSupported(context.FilePath.value(), fileType);
        if (!supported)
        {
            result.Result = FontResult::Error;
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
            AICLI_LOG(Core, Error, << L"Font file is not supported: " << context.FilePath.value() << " - " << APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
            return result;
        }

        // Determine locations for install
        const auto& installFolderPath = GetFontFileInstallPath(context);
        const auto& installRegistryPath = GetFontRegistryPath(context);
        const auto& destinationFilePath = installFolderPath / context.FilePath.value().filename();

        // Check if already installed
        //   1) Check registry key existence
        //   2) Check file existence
        //   Note presence if forced install.

        // Font install step 1: Copy file to winget package identifiable location.
        // TODO: Handle case of two font files in same package with the same filename. Is this possible?
        // TODO: Handle forced install and files-in-use.
        try
        {
            AICLI_LOG(Core, Info, << "Moving " << context.FilePath.value().filename() << " to " << destinationFilePath);
            std::filesystem::create_directories(destinationFilePath);
            AppInstaller::Filesystem::RenameFile(context.FilePath.value(), destinationFilePath);
        }
        catch (const wil::ResultException& e)
        {
            result.Result = FontResult::Error;
            result.HResult = e.GetErrorCode();
            AICLI_LOG(Core, Error, << L"Failed moving font file: " << e.GetFailureInfo().pszMessage << " - " << e.GetErrorCode());

            // TODO: Rollback
            return result;
        }

        // Set Registry key to winget package identifiable location.
        try
        {
            const auto& fontTitle = GetFontFileTitle(context.FilePath.value());
            AICLI_LOG(Core, Info, << "Adding " << AppInstaller::Utility::ConvertToUTF8(fontTitle) << " to " << AppInstaller::Utility::ConvertToUTF8(installRegistryPath));
            auto hive = context.Scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
            auto key = Registry::Key::Create(hive, installRegistryPath);
            key.SetValue(fontTitle, destinationFilePath, REG_SZ);
        }
        catch (const wil::ResultException& e)
        {
            result.Result = FontResult::Error;
            result.HResult = e.GetErrorCode();
            AICLI_LOG(Core, Error, << L"Failed setting registry: " << e.GetFailureInfo().pszMessage << " - " << e.GetErrorCode());

            // TODO: Rollback
            return result;
        }

        // Add Font Resource to the session.
        // Commenting out due to AddFontResource linking issue.
        auto fontsAdded = ::AddFontResource(destinationFilePath.c_str());
        if (fontsAdded == 0)
        {
            // AddFontResource does not add additional information for us, so we dont know why they were not added,
            // only that they were not added. At this point the "install" is successful, but we were not able to
            // add the font to the system, which means subsequent adds on session start may also fail, but is not
            // guaranteed to fail. We will note it in the log and carry on.
            AICLI_LOG(Core, Info, << L"Failed to add font resource: " << destinationFilePath);
        }
        else
        {
            AICLI_LOG(Core, Info, << L"Added " << fontsAdded << " fonts to the session.");

            // Notifying the system of a font change is optional, as we do not want to slam the system with
            // messages when installing a large number of fonts. This is used for a single font file install.
            if (notifySystem)
            {
                // Notifying the system of a font change is best-effort and does not affect the install state.
                LOG_IF_FAILED(NotifyFontChange());
            }
        }

        result.Result = FontResult::Success;
        return result;
    }

    // Uninstall is at the package level. We remove all fonts for the specified package name.
    // A package in this context is just the namespace under the WinGet subfolders/subkeys.
    FontOperationResult UninstallFontFile(FontContext& context, const bool notifySystem)
    {
        DBG_UNREFERENCED_PARAMETER(context);
        DBG_UNREFERENCED_PARAMETER(notifySystem);

        FontOperationResult result;

        // Uninstall Steps
        // 1) Remove all font resources for every font in the package.
        // 2) Remove the registry subkey
        // 3) Remove the subfolder with all of the font files, or move & mark for deletion next reboot if in-use.
        // 4) Send update that fonts have changed.

        return result;
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
        if (!std::filesystem::exists(filePath))
        {
            fileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
            return false;
        }

        wil::com_ptr<IDWriteFontFile> fontFile;
        THROW_IF_FAILED(m_factory->CreateFontFileReference(filePath.c_str(), NULL, &fontFile));

        BOOL isSupported;
        DWRITE_FONT_FACE_TYPE faceType;
        UINT32 numOfFaces;
        THROW_IF_FAILED(fontFile->Analyze(&isSupported, &fileType, &faceType, &numOfFaces));
        return isSupported;
    }

    std::filesystem::path FontCatalog::GetRootFontPath(Manifest::ScopeEnum scope)
    {
        if (scope == Manifest::ScopeEnum::Machine)
        {
            return Runtime::GetPathTo(Runtime::PathName::FontsMachineInstallLocation);
        }
        else
        {
            return Runtime::GetPathTo(Runtime::PathName::FontsUserInstallLocation);
        }
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

    // This will create an inventory of all known permanently installed fonts to the user.
    // A font is "permanently installed" if it is present in the Font registry for the machine or the user.
    // This will not include fonts that are temporarily installed for the session.
    std::vector<FontFileInfo> GetInstalledFontFiles()
    {
        auto fontFiles = std::vector<FontFileInfo>();

        // Iterate through scopes for machine and user
        std::vector<Manifest::ScopeEnum> scopes = { Manifest::ScopeEnum::Machine, Manifest::ScopeEnum::User };
        for (const auto& scope : scopes)
        {
            auto hive = scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
            auto root = Registry::Key::OpenIfExists(hive, std::wstring{ s_FontsPathSubkey });

            // There are two supported scenarios for tracking subkeys.
            // 1) We created the subkey, therefore it is a winget installed font.
            // 2) A package created the subkey for a package-deployed font
            // We assume that all subkeys not the WinGet key are packaged keys. It is not guaranteed that
            // a package created the font, but we will assume that it is to be safe in how we handle them.
            for (const auto& subkey : root)
            {
                auto subkeyName = ConvertToUTF16(subkey.Name());
                auto subkeyKey = subkey.Open();
                if (subkeyName == s_FontsWinGetPrefix)
                {
                    // Assum all subkeys are WinGet Packages
                }
                else
                {
                    // Assume all subkeys are fonts installed by a UAP package.
                    for (const auto& uapPackageValue : subkeyKey.Values())
                    {
                        auto value = uapPackageValue.Value();
                        if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                        {
                            continue;
                        }

                        auto context = FontContext();
                        context.Title = ConvertToUTF16(uapPackageValue.Name());
                        context.Scope = scope;
                        context.InstallerSource = InstallerSource::UWP;
                        context.FilePath = { value->GetValue<Registry::Value::Type::String>() };
                        auto fontFile = CreateFontFileInfo(context);
                        fontFiles.push_back(std::move(fontFile));
                    }
                }
            }

            // All remaining values are externally installed fonts.
            for (const auto& rootValue : root.Values())
            {
                auto value = rootValue.Value();
                if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                {
                    continue;
                }

                auto context = FontContext();
                context.Title = ConvertToUTF16(rootValue.Name());
                context.Scope = scope;
                context.InstallerSource = InstallerSource::Unknown;
                context.FilePath = { value->GetValue<Registry::Value::Type::String>() };
                auto fontFile = CreateFontFileInfo(context);
                fontFiles.push_back(std::move(fontFile));
            }
        }

        return fontFiles;
    }
}
