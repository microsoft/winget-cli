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
    namespace
    {
        constexpr std::wstring_view s_FontsPathSubkey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
        constexpr std::wstring_view s_FontsUserInstallFolder = L"Microsoft\\Windows\\Fonts";
        constexpr std::wstring_view s_TrueType = L" (TrueType)";
        constexpr std::wstring_view s_UnknownInstallerPrefixUser = L"U\\F\\";
        constexpr std::wstring_view s_UnknownInstallerPrefixMachine = L"M\\F\\";
        constexpr std::wstring_view s_RegistrySeparator = L"\\";
        constexpr std::wstring_view s_FontsWinGetPrefix = L"winget_v1";
        const int s_RemoveFontResourceMaxTries = 1000;

        std::wstring GetFontFileTitle(const std::filesystem::path& fontFilePath)
        {
            // This code can fail in a number of ways, including if the file isn't actually
            // a font file or does not have a title.
            wil::com_ptr<IPropertyStore> pPropertyStore;
            THROW_IF_FAILED(SHGetPropertyStoreFromParsingName(fontFilePath.c_str(), nullptr, GPS_DEFAULT, IID_PPV_ARGS(&pPropertyStore)));
            PROPVARIANT prop;
            PropVariantInit(&prop);
            THROW_IF_FAILED(pPropertyStore->GetValue(PKEY_Title, &prop));
            std::wstring title;
            if (prop.pwszVal)
            {
                title = prop.pwszVal;
            }
            THROW_IF_FAILED(PropVariantClear(&prop));
            return title;
        }

        void EnsurePackageInformation(const FontContext& context)
        {
            if (context.PackageId.has_value() && !context.PackageId.value().empty() &&
                context.PackageVersion.has_value() && !context.PackageVersion.value().empty())
            {
                return;
            }

            // This is a programming error if we reach this point where the package identifer cannot be created or derived.
            THROW_HR_MSG(E_UNEXPECTED, "Package Id and Version must be provided and non-empty.");
        }

        std::wstring GetFontRegistryPath(const FontContext& context)
        {
            // Registry path for a font is well-defined based on installer source and package information.
            auto path = std::wstringstream();
            path << s_FontsPathSubkey;

            switch (context.InstallerSource)
            {
            case InstallerSource::Unknown:
                // Unknown installer could be anywhere, assume it is in the default fonts location.
                break;
            case InstallerSource::WinGet:
                // WinGet path adds the WinGet prefix + package id + version.
                EnsurePackageInformation(context);
                path << s_RegistrySeparator << s_FontsWinGetPrefix << s_RegistrySeparator << context.PackageId.value() <<
                    s_RegistrySeparator << context.PackageVersion.value();
                break;
            default:
                THROW_HR_MSG(E_UNEXPECTED, "Undefined InstallerSource.");
            }

            return path.str();
        }

        std::filesystem::path GetRootFontPath(Manifest::ScopeEnum scope)
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

        std::filesystem::path GetFontFileInstallPath(const FontContext& context)
        {
            auto path = GetRootFontPath(context.Scope);

            switch (context.InstallerSource)
            {
            case InstallerSource::Unknown:
                // Unknown installer is assumed to be installed in the default location for machine.
                // For a user installed font it could be anywhere.
                break;
            case InstallerSource::WinGet:
                // WinGet installed packages have two formats depending on the scope.
                // For Per-user we use subfolders for better robustness and cleaner format.
                // For Per-machine we must use a single folder due to a system requirement,
                // so the root path is the machine install path.
                if (context.Scope == Manifest::ScopeEnum::User)
                {
                    EnsurePackageInformation(context);
                    path /= s_FontsWinGetPrefix;
                    path /= context.PackageId.value();
                    path /= context.PackageVersion.value();
                }

                break;
            default:
                THROW_HR_MSG(E_UNEXPECTED, "Undefined InstallerSource.");
            }

            return path;
        }

        // For machine-installed fonts the files all reside in the same folder. We need a unique name that is reasonably human
        // readable. The way system fonts normally do this is by appending a number to the end of the file stem. We will follow
        // the same pattern by using the original file and adding a number until we arrive at a unique filename.
        std::filesystem::path GetUniquePathForDestination(const std::filesystem::path& source, const std::filesystem::path& destination)
        {
            const auto stem = source.stem();
            const auto ext = source.extension();
            auto uniqueName = source.filename();
            auto candidatePath = destination / uniqueName;
            int index = 0;
            while (std::filesystem::exists(candidatePath))
            {
                std::filesystem::path appendId = { std::to_string(index) };
                uniqueName = stem;
                uniqueName += appendId;
                uniqueName += ext;
                candidatePath = destination / uniqueName;
                index++;
            }

            return candidatePath;
        }

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

        void RemoveAllFontResources(const std::filesystem::path& filePath)
        {
            // The recommended uninstall method of a font is to call RemoveFontResource until it fails,
            // This is not guaranteed to remove the file from use, but it is the best we have.
            // https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-removefontresourcea
            int i = 0;
            while (::RemoveFontResource(filePath.c_str()))
            {
                // Let us not loop endlessly.
                if (++i >= s_RemoveFontResourceMaxTries)
                {
                    break;
                }
            }
        }

        void NotifyFontChange()
        {
            // Send the WM_FONTCHANGE message so the system and apps know that there has been a font change.
            // Sometimes this does not have an error when it returns non-zero. To avoid random assert failures
            // we will not try to log failures for this call. If it fails it does not affect install state.
            auto sendResult = ::SendNotifyMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
            AICLI_LOG(Core, Info, << L"Notified system of font change: " << sendResult);
        }

        // Checks the registry path for the file specified, and returns the key value if it finds one.
        std::optional<std::wstring> CheckRegistryForFontFileReference(const std::wstring& registryPath, const std::filesystem::path& filePath, Manifest::ScopeEnum scope)
        {
            try
            {
                const auto& hive = scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
                const auto& key = Registry::Key::OpenIfExists(hive, registryPath, 0UL, KEY_ALL_ACCESS);
                if (key)
                {
                    // Check for the file being present in this key.
                    for (const auto& valueRef : key.Values())
                    {
                        const auto& value = valueRef.Value();
                        if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                        {
                            continue;
                        }
                        const auto& valueName = ConvertToUTF16(valueRef.Name());
                        const std::filesystem::path& valuePath = { value->GetValue<Registry::Value::Type::String>() };
                        if (valuePath.is_relative())
                        {
                            // Relative value is OK, just compare the filenames.
                            // We will assume this is in the standard font paths.
                            if (valuePath.filename() == filePath.filename())
                            {
                                return valueName;
                            }
                        }
                        else
                        {
                            if (valuePath == filePath)
                            {
                                return valueName;
                            }
                        }
                    }
                }
            }
            CATCH_LOG();

            return std::nullopt;
        }

        // The package registry for machine installed fonts stores the package filename as a registry value.
        // This is so we can map the original package filename to the installed filename since they may be different.
        std::optional<std::filesystem::path> GetRegisteredMachineFontInstallPath(const std::wstring& registryPath, const std::filesystem::path& filePath)
        {
            try
            {
                const auto& key = Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, registryPath, 0UL, KEY_ALL_ACCESS);
                if (key)
                {
                    for (const auto& valueRef : key.Values())
                    {
                        const auto& value = valueRef.Value();
                        if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                        {
                            continue;
                        }

                        const auto& valueName = ConvertToUTF16(valueRef.Name());
                        if (valueName == filePath.filename())
                        {
                            const std::filesystem::path& valuePath = { value->GetValue<Registry::Value::Type::String>() };
                            return valuePath;
                        }
                    }
                }
            }
            CATCH_LOG();

            return std::nullopt;
        }
    }

    FontResult FontOperationResult::Result() const
    {
        if (FAILED(HResult))
        {
            return FontResult::Error;
        }
        else
        {
            return FontResult::Success;
        }
    }

    void FontContext::AddPackageFile(const std::filesystem::path& filePath)
    {
        if (!PackageFiles.has_value())
        {
            PackageFiles = std::vector<std::filesystem::path>();
        }

        PackageFiles.value().push_back(filePath);
    }

    // Creates font file info used for install, validation, and enumeration.
    FontFileInfo CreateFontFileInfo(const FontContext& context, const std::filesystem::path& filePath, const std::optional<std::wstring>& title)
    {
        auto fileInfo = FontFileInfo();
        fileInfo.FilePath = filePath;
        fileInfo.PackageIdentifier = context.PackageIdentifier;
        fileInfo.PackageId = context.PackageId;
        fileInfo.PackageVersion = context.PackageVersion;
        fileInfo.InstallerSource = context.InstallerSource;
        fileInfo.Scope = context.Scope;
        if (title.has_value())
        {
            fileInfo.Title = title.value();
        }

        switch (fileInfo.InstallerSource)
        {
        case InstallerSource::WinGet:
            EnsurePackageInformation(context);
            break;

        case InstallerSource::Unknown:

            // For unknown installers, give them a name that reflects this so
            // they can be referenced.
            // We will assume the title is provided for these, since we do
            // not install them, they must already exist on the system or
            // have a reference in the registry.
            if (!fileInfo.PackageIdentifier.has_value() || fileInfo.PackageIdentifier.value().empty())
            {
                if (fileInfo.Scope == Manifest::ScopeEnum::Machine)
                {
                    fileInfo.PackageIdentifier = s_UnknownInstallerPrefixMachine.data() + filePath.filename().wstring();
                }
                else
                {
                    fileInfo.PackageIdentifier = s_UnknownInstallerPrefixUser.data() + filePath.filename().wstring();
                }
            }

            break;

        default:
            THROW_HR_MSG(E_UNEXPECTED, "Undefined InstallerSource.");
        }

        // For font file paths in the registry, we may have a relative filename (because the path is assumed).
        if (fileInfo.FilePath.is_relative())
        {
            // Relative path is assumed to be relative to the default location for the scope.
            // We are assuming that a relative file path input refers to an already-installed file.
            auto fullPath = GetRootFontPath(fileInfo.Scope);
            fullPath /= fileInfo.FilePath;
            fileInfo.FilePath = std::move(fullPath);
        }

        // Get information about the font file itself.
        if (std::filesystem::exists(fileInfo.FilePath))
        {
            auto fontCatalog = FontCatalog();
            fileInfo.WinGetSupported = fontCatalog.IsFontFileSupported(fileInfo.FilePath, fileInfo.FileType);

            if (fileInfo.WinGetSupported && fileInfo.Title.empty())
            {
                try
                {
                    fileInfo.Title = GetFontFileTitle(fileInfo.FilePath);
                }
                CATCH_LOG();
            }
        }
        else
        {
            fileInfo.WinGetSupported = false;
            fileInfo.FileType = DWRITE_FONT_FILE_TYPE_UNKNOWN;
        }

        if (fileInfo.Title.empty())
        {
            // If the title is still empty (such as failure to set it from the File), use the Filename
            fileInfo.Title = fileInfo.FilePath.filename().wstring();
        }

        fileInfo.RegistryPackagePath = GetFontRegistryPath(context);
        if (fileInfo.Scope == Manifest::ScopeEnum::Machine && fileInfo.InstallerSource == InstallerSource::WinGet)
        {
            // Machine fonts from WinGet still install into the Font folder but we track package source separately
            // due to limited information in the Font registry and all font files being in the same key and folder.
            fileInfo.RegistryInstallPath = s_FontsPathSubkey;
        }
        else
        {
            // User-installed and non-Winget fonts have same registry package path and install path.
            fileInfo.RegistryInstallPath = fileInfo.RegistryPackagePath;
        }

        // Check if the file is already in the install path, in which case this is a query.
        auto const& installPath = GetFontFileInstallPath(context);
        if (installPath == fileInfo.FilePath.parent_path())
        {
            fileInfo.InstallPath = fileInfo.FilePath;
        }
        else
        {
            if (fileInfo.InstallerSource == InstallerSource::WinGet && fileInfo.Scope == Manifest::ScopeEnum::Machine)
            {
                // Machine installed WinGet fonts must have a non-conflicting filename in the root font folder.
                // To ensure a unique font name and allow different versions to exist side by side, the install file
                // name may be a different name from the original package install. To account for this, the Package
                // registry location uses the original filename as the registry value name and the value as the
                // resulting path. If we are installing a machine font that already exists, we must check to see
                // if this file already has a mapping defined.
                const auto& installedFilePath = GetRegisteredMachineFontInstallPath(fileInfo.RegistryPackagePath.value(), fileInfo.FilePath.filename());
                if (installedFilePath.has_value())
                {
                    fileInfo.InstallPath = installedFilePath.value();
                }
                else
                {
                    // File not already defined, we need to create a non-conflicting filename.
                    // Machine-installed WinGet must provide a non-conflicting filename to the root install location.
                    fileInfo.InstallPath = GetUniquePathForDestination(fileInfo.FilePath, installPath);
                }
            }
            else
            {
                fileInfo.InstallPath = installPath / fileInfo.FilePath.filename();
            }
        }

        // If our font file is regstered it will exist in the install path and have a registry value.
        const auto& registryTitle = CheckRegistryForFontFileReference(fileInfo.RegistryInstallPath.value(), fileInfo.InstallPath, fileInfo.Scope);

        if (fileInfo.Title.empty())
        {
            // Use the registry title or filename if we still dont' know.
            if (registryTitle.has_value())
            {
                fileInfo.Title = registryTitle.value();
            }
            else
            {
                fileInfo.Title = fileInfo.FilePath.filename().wstring();
            }
        }

        fileInfo.IsFontFileInstalled = std::filesystem::exists(fileInfo.InstallPath) ? true : false;
        fileInfo.IsFontFileRegistered = registryTitle.has_value() ? true : false;
        if (fileInfo.IsFontFileInstalled && fileInfo.IsFontFileRegistered)
        {
            fileInfo.Status = FontStatus::OK;
        }
        else if (fileInfo.IsFontFileInstalled || fileInfo.IsFontFileRegistered)
        {
            fileInfo.Status = FontStatus::Corrupt;
        }
        else
        {
            fileInfo.Status = FontStatus::Absent;
        }

        return fileInfo;
    }

    FontValidationResult ValidateFontPackage(FontContext& context)
    {
        EnsurePackageInformation(context);

        auto result = FontValidationResult();

        AICLI_LOG(Core, Info, << L"Validating font package: " << context.PackageIdentifier.value_or(L"<Unknown>").c_str());

        auto fontFileInfos = std::vector<FontFileInfo>();
        try
        {
            if (context.PackageFiles.has_value())
            {
                // Create font file info for each package file
                for (const auto& file : context.PackageFiles.value())
                {
                    result.FontFileInfos.push_back(CreateFontFileInfo(context, file));
                }

                // We create an overall status for the validation with the following determination:
                // If ALL of the files are OK, the package is OK.
                // If ALL of the files are Absent, the package is Absent.
                // If there is any combination, the package is Corrupt.
                // We must check each file info to see if the package is supported by WinGet.
                // If any file is unsupported then there are unsupported fonts in the package.
                for (const auto& fontFileInfo : result.FontFileInfos)
                {
                    if (!fontFileInfo.WinGetSupported)
                    {
                        result.HasUnsupportedFonts = true;
                    }

                    if (fontFileInfo.Status != result.Status)
                    {
                        if (result.Status == FontStatus::Unknown)
                        {
                            // This is the first status we've seen, make it the default.
                            result.Status = fontFileInfo.Status;
                        }
                        else
                        {
                            // Previous status now differs from current status, package is corrupt.
                            result.Status = FontStatus::Corrupt;
                        }
                    }
                }
            }

            result.Result = FontResult::Success;
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            result.HResult = APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED;
            result.Result = FontResult::Error;
        }

        return result;
    }

    FontOperationResult InstallFontPackage(FontContext& context)
    {
        EnsurePackageInformation(context);

        FontOperationResult result;

        if (context.InstallerSource != InstallerSource::WinGet)
        {
            throw std::logic_error("Only WinGet format of font install is supported.");
        }

        if (!context.PackageFiles.has_value() || context.PackageFiles.value().size() == 0)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_FOUND);
            AICLI_LOG(Core, Error, << L"Font package has no files: " << context.PackageId.value().c_str() << " - " << result.HResult);
            return result;
        }

        // Validate the package data was all processed successfully.
        const auto& validationResult = ValidateFontPackage(context);
        if (validationResult.Result != FontResult::Success)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
            AICLI_LOG(Core, Error, << L"Font package validation failed: " << context.PackageId.value().c_str() << " - " << validationResult.HResult);
            return result;
        }

        // Check for unsupported font files.
        if (validationResult.HasUnsupportedFonts)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
            AICLI_LOG(Core, Error, << L"Font package has unsupported fonts: " << context.PackageId.value().c_str() << " - " << result.HResult);
            return result;
        }

        // Check for package already installed and correct unless this is a force install.
        if (validationResult.Status == FontStatus::OK && !context.Force)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_ALREADY_INSTALLED);
            AICLI_LOG(Core, Info, << L"Font package is already installed and in a good state.: " << context.PackageId.value().c_str() << " - " << result.HResult);
            return result;
        }

        // We will attempt a cleanup / force install if either this is a forced install or the package was not in a good state.
        if (validationResult.Status == FontStatus::Corrupt || context.Force)
        {
            // Force install of a font can have problems because if the font is already there then it
            // may be in use. The scenarios for using force is if a prior install attempt failed, so
            // we will try to clean up the existing font registration so it may be successful this time.
            AICLI_LOG(Core, Info, << L"Package is corrupt or forced install, attempting to remove any existing registration.");
            auto uninstallResult = UninstallFontPackage(context);
            if (uninstallResult.Result() != FontResult::Success)
            {
                result.HResult = uninstallResult.HResult;
                AICLI_LOG(Core, Error, << L"Font cleanup uninstall failed: " << context.PackageId.value().c_str() << L"-" << uninstallResult.HResult);
                return result;
            }
        }

        AICLI_LOG(Core, Info, << L"Starting install of " << context.PackageId.value().c_str());

        try
        {
            // Install each file from the file info.
            for (const auto& fontFileInfo : validationResult.FontFileInfos)
            {
                // Font install step 1: Copy file to winget package identifiable location.
                AICLI_LOG(Core, Info, << L"Moving " << fontFileInfo.FilePath << L" to " << fontFileInfo.InstallPath);
                if (!std::filesystem::exists(fontFileInfo.InstallPath.parent_path()))
                {
                    std::filesystem::create_directories(fontFileInfo.InstallPath.parent_path());
                }

                // The file should not be present, but if it is, do another attempt to remove it.
                if (std::filesystem::exists(fontFileInfo.InstallPath))
                {
                    // Try to remove font resource to avoid file-in-use.
                    RemoveAllFontResources(fontFileInfo.InstallPath);
                    if (!std::filesystem::remove(fontFileInfo.InstallPath))
                    {
                        AICLI_LOG(Core, Error, << L"Font file already exists and was unable to be removed.");
                        THROW_HR(APPINSTALLER_CLI_ERROR_FONT_INSTALL_FAILED);
                    }
                }

                if (context.Scope == Manifest::ScopeEnum::Machine)
                {
                    // Using std::filesystem::rename does not work when installing into the system Fonts
                    // folder; the system does not recognize the font files. std::filesystem::copy_file does work.
                    std::filesystem::copy_file(fontFileInfo.FilePath, fontFileInfo.InstallPath);
                    std::filesystem::remove(fontFileInfo.FilePath);
                }
                else
                {
                    // We prefer to use rename where it works.
                    AppInstaller::Filesystem::RenameFile(fontFileInfo.FilePath, fontFileInfo.InstallPath);
                }

                AICLI_LOG(Core, Info, << L"Adding " << fontFileInfo.Title.c_str() << L" to " << fontFileInfo.InstallPath);
                if (context.Scope == Manifest::ScopeEnum::User)
                {
                    auto key = Registry::Key::Create(HKEY_CURRENT_USER, fontFileInfo.RegistryInstallPath.value());
                    key.SetValue(fontFileInfo.Title, fontFileInfo.InstallPath, REG_SZ);
                }
                else
                {
                    // Machine install we set two keys, one for sourcing, and one for the actual install.
                    // The value name is the source filename so we can map this later for validation.
                    auto sourceKey = Registry::Key::Create(HKEY_LOCAL_MACHINE, fontFileInfo.RegistryPackagePath.value(), 0UL, KEY_ALL_ACCESS);
                    sourceKey.SetValue(fontFileInfo.FilePath.filename(), fontFileInfo.InstallPath, REG_SZ);

                    // Machine font install uses relative filename. Use filename as value to avoid collisions in the key.
                    auto installKey = Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, fontFileInfo.RegistryInstallPath.value(), 0UL, KEY_ALL_ACCESS);
                    installKey.SetValue(fontFileInfo.InstallPath.filename(), fontFileInfo.InstallPath.filename(), REG_SZ);
                }

                // Add Font Resource to the session.
                const auto& fontsAdded = ::AddFontResource(fontFileInfo.InstallPath.c_str());
                if (fontsAdded == 0)
                {
                    // AddFontResource does not add additional information for us, so we dont know why they were not added,
                    // only that they were not added. At this point the "install" is successful, but we were not able to
                    // add the font to the system, which means subsequent adds on session start may also fail, but is not
                    // guaranteed to fail. We will note it in the log and carry on.
                    AICLI_LOG(Core, Warning, << L"Failed to add font resource: " << fontFileInfo.InstallPath);
                }
                else
                {
                    AICLI_LOG(Core, Info, << L"Added " << fontsAdded << L" fonts to the session.");
                }
            }

            result.HResult = S_OK;
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << L"Install failed for " << context.PackageId.value().c_str() << L", attempting rollback.");

            try
            {
                // Rollback. In this case, rollback is uninstall, which should remove any partial installation.
                // This is best-effort.
                auto rollbackResult = UninstallFontPackage(context);
                if (rollbackResult.Result() == FontResult::Success)
                {
                    AICLI_LOG(Core, Info, << L"Rollback for " << context.PackageId.value().c_str() << L" successful.");
                }
                else
                {
                    AICLI_LOG(Core, Error, << L"Rollback for " << context.PackageId.value().c_str() << L" failed: " << rollbackResult.HResult);
                }
            }
            CATCH_LOG();

            result.HResult = APPINSTALLER_CLI_ERROR_FONT_INSTALL_FAILED;
        }

        // Regardless of the result there's likely some changes that occurred.
        NotifyFontChange();
        return result;
    }

    FontOperationResult UninstallFontPackage(FontContext& context)
    {
        FontOperationResult result;

        if (context.InstallerSource != InstallerSource::WinGet)
        {
            throw std::logic_error("Only WinGet format of font package uninstall is supported.");
        }

        EnsurePackageInformation(context);

        const auto& installFolderPath = GetFontFileInstallPath(context);
        const auto& installPackagePath = GetFontRegistryPath(context);
        std::vector<std::filesystem::path> filesToRemove;

        try
        {
            auto hive = context.Scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
            auto key = Registry::Key::OpenIfExists(hive, installPackagePath, 0ul, KEY_ALL_ACCESS);

            if (key)
            {
                // Assume all values in this key are fonts related to this package.
                for (const auto& fontEntry : key.Values())
                {
                    auto value = fontEntry.Value();
                    if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                    {
                        // Not a valid entry, nothing to do here.
                        continue;
                    }

                    std::filesystem::path filePath = { value->GetValue<Registry::Value::Type::String>() };
                    if (!std::filesystem::exists(filePath))
                    {
                        // File doesn't exist, nothing to do here.
                        continue;
                    }

                    // The font may be in use by the system or other apps, it needs to be removed from
                    // from use or at least attempted to be removed from use.
                    RemoveAllFontResources(filePath);
                    filesToRemove.push_back(filePath);
                }

                // Delete the key
                if (!Registry::Key::DeleteTree(hive, installPackagePath))
                {
                    AICLI_LOG(Core, Warning, << L"Failed removing registry tree " << installPackagePath.c_str());
                }

                if (context.Scope == Manifest::ScopeEnum::Machine)
                {
                    // Machine installed fonts also have an entry in the System font registry.
                    // We have to find the values that have the filename as data.
                    auto machineFontRoot = Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, s_FontsPathSubkey.data(), 0ul, KEY_ALL_ACCESS);

                    for (const auto& file : filesToRemove)
                    {
                        machineFontRoot.DeleteValue(file.filename());
                    }
                }
            }
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();

            // For uninstall we will log the error and continue trying to remove it, since we have partial remove and are in an unknown state.
            AICLI_LOG(Core, Error, << L"Failed removing fonts in the registry.");
        }

        // Remove any font files.
        for (const auto& file : filesToRemove)
        {
            try
            {
                if (std::filesystem::exists(file))
                {
                    // TODO: Add robustness for files-in-use scenarios.
                    std::filesystem::remove(file);
                }
            }
            catch (...)
            {
                LOG_CAUGHT_EXCEPTION();
                AICLI_LOG(Core, Error, << L"Failed removing font files.");
                result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED);
            }
        }

        if (context.Scope == Manifest::ScopeEnum::User)
        {
            // User installed fonts also have an install folder, we will clean that up as well.
            if (std::filesystem::exists(installFolderPath))
            {
                try
                {
                    const auto& parent = installFolderPath.parent_path();
                    std::filesystem::remove_all(installFolderPath);

                    // This may have been the only version installed, so remove parent if empty.
                    if (std::filesystem::is_empty(parent))
                    {
                        std::filesystem::remove(parent);
                    }
                }
                catch (...)
                {
                    // Font files have already been removed at this point, so failure to delete
                    // this folder is not a failure of removing the fonts.
                    LOG_CAUGHT_EXCEPTION();
                    AICLI_LOG(Core, Error, << L"Failed removing font folders.");
                }
            }
        }

        // Notify system of font changes.
        NotifyFontChange();
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

        std::string value = ConvertToUTF8(GetLocalizedStringFromFont(fontVersion));
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

    // This will create an inventory of all known permanently installed fonts to the user.
    // A font is "permanently installed" if it is present in the Font registry for the machine or the user.
    // This will not include fonts that are temporarily installed for the session.
    std::vector<FontFileInfo> GetInstalledFontFiles()
    {
        auto fontFiles = std::vector<FontFileInfo>();

        try
        {
            auto wingetMachineFonts = std::unordered_map<std::filesystem::path, bool>();

            // Iterate through scopes for machine and user
            std::vector<Manifest::ScopeEnum> scopes = { Manifest::ScopeEnum::Machine, Manifest::ScopeEnum::User };
            for (const auto& scope : scopes)
            {
                auto hive = scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
                auto root = Registry::Key::OpenIfExists(hive, std::wstring{ s_FontsPathSubkey });

                // There are two supported scenarios for tracking sub-keys.
                // 1) We created the subkey, therefore it is a winget installed font.
                // 2) A package created the subkey for a package-deployed font
                // We assume that all sub-keys not the WinGet key are packaged keys. It is not guaranteed that
                // a package created the font, but we will assume that it is to be safe in how we handle them.
                for (const auto& subkey : root)
                {
                    auto subkeyName = ConvertToUTF16(subkey.Name());
                    auto subkeyKey = subkey.Open();
                    if (subkeyName == s_FontsWinGetPrefix)
                    {
                        // Assume all sub-keys are WinGet Packages
                        for (const auto& packageSubKey : subkeyKey)
                        {
                            // All subkeys should be versions of the package.
                            auto wingetPackageSubKey = packageSubKey.Open();
                            for (const auto& versionSubKey : wingetPackageSubKey)
                            {
                                auto packageId = ConvertToUTF16(packageSubKey.Name());
                                auto version = ConvertToUTF16(versionSubKey.Name());
                                auto packageVersionSubKey = versionSubKey.Open();
                                for (const auto& versionValue : packageVersionSubKey.Values())
                                {
                                    // Values are the files.
                                    auto value = versionValue.Value();
                                    if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                                    {
                                        continue;
                                    }

                                    std::filesystem::path filePath = { value->GetValue<Registry::Value::Type::String>() };
                                    if (scope == Manifest::ScopeEnum::Machine)
                                    {
                                        // Capture the filename for winget machine fonts so we can avoid duplicates.
                                        wingetMachineFonts[filePath.filename()] = true;
                                    }

                                    auto context = FontContext();
                                    context.Scope = scope;
                                    context.InstallerSource = InstallerSource::WinGet;
                                    context.PackageId = packageId;
                                    context.PackageVersion = version;
                                    auto fontFile = CreateFontFileInfo(context, filePath, ConvertToUTF16(versionValue.Name()));
                                    fontFiles.push_back(std::move(fontFile));
                                }
                            }
                        }
                    }
                }

                // All remaining values in the root are externally installed fonts.
                for (const auto& rootValue : root.Values())
                {
                    auto value = rootValue.Value();
                    if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                    {
                        continue;
                    }

                    std::filesystem::path filePath = { value->GetValue<Registry::Value::Type::String>() };
                    if (scope == Manifest::ScopeEnum::Machine)
                    {
                        // Skip it if this is already identified in the WinGet machine fonts.
                        if (wingetMachineFonts[filePath.filename()])
                        {
                            continue;
                        }
                    }

                    auto context = FontContext();
                    context.Scope = scope;
                    context.InstallerSource = InstallerSource::Unknown;
                    auto fontFile = CreateFontFileInfo(context, filePath, ConvertToUTF16(rootValue.Name()));
                    fontFiles.push_back(std::move(fontFile));
                }
            }
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << L"Failed getting font file information.");
        }

        return fontFiles;
    }

    std::vector<FontPackageInfo> GetInstalledFontPackages(Manifest::ScopeEnum scope)
    {
        auto fontPackages = std::vector<FontPackageInfo>();

        try
        {
            auto hive = scope == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
            auto root = Registry::Key::OpenIfExists(hive, s_FontsPathSubkey.data(), 0UL, KEY_ALL_ACCESS);

            for (const auto& subkey : root)
            {
                auto subkeyName = ConvertToUTF16(subkey.Name());
                auto subkeyKey = subkey.Open();
                if (subkeyName == s_FontsWinGetPrefix)
                {
                    // Assume all sub-keys are WinGet Packages
                    for (const auto& packageSubKey : subkeyKey)
                    {
                        try
                        {
                            // All subkeys should be versions of the package.
                            auto wingetPackageSubKey = packageSubKey.Open();
                            for (const auto& versionSubKey : wingetPackageSubKey)
                            {
                                auto packageId = ConvertToUTF16(packageSubKey.Name());
                                auto version = ConvertToUTF16(versionSubKey.Name());
                                auto context = FontContext();
                                context.Scope = scope;
                                context.InstallerSource = InstallerSource::WinGet;
                                context.PackageId = packageId;
                                context.PackageVersion = version;

                                auto packageVersionSubKey = versionSubKey.Open();
                                for (const auto& versionValue : packageVersionSubKey.Values())
                                {
                                    auto value = versionValue.Value();
                                    if (!value.has_value() || (value.value().GetType() != Registry::Value::Type::String))
                                    {
                                        continue;
                                    }

                                    std::filesystem::path filePath = { value->GetValue<Registry::Value::Type::String>() };
                                    context.AddPackageFile(filePath);
                                }

                                auto validationResult = ValidateFontPackage(context);
                                if (validationResult.Status != FontStatus::Absent)
                                {
                                    auto packageInfo = FontPackageInfo();
                                    packageInfo.Scope = scope;
                                    packageInfo.Status = validationResult.Status;
                                    packageInfo.PackageId = packageId;
                                    packageInfo.PackageVersion = version;
                                    fontPackages.push_back(std::move(packageInfo));
                                }
                            }
                        }
                        catch (...)
                        {
                            // If we have bad data in the registry for this entry, log it and skip.
                            LOG_CAUGHT_EXCEPTION();
                            AICLI_LOG(Core, Error, << L"Failed getting font package information for: " << packageSubKey.Name().c_str());
                            continue;
                        }
                    }
                }
            }
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << L"Failed getting font package information.");
        }

        return fontPackages;
    }
}
