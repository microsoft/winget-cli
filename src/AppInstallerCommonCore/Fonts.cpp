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
#include <winget/ManifestValidation.h>
#include <winget/Registry.h>
#include <ShObjIdl_core.h>
#include <propkey.h>
#include <wingdi.h>
#include <functional>

using namespace AppInstaller::Utility;

namespace AppInstaller::Fonts
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    using GetFontRegistryHiveFunc = std::function<Registry::Key(Manifest::ScopeEnum)>;
    static GetFontRegistryHiveFunc s_GetFontRegistryHiveOverride;
    static bool s_FontSessionChangesDisabled = false;

    void TestHook_SetGetFontRegistryHive_Override(GetFontRegistryHiveFunc value)
    {
        s_GetFontRegistryHiveOverride = std::move(value);
    }

    void TestHook_SetFontSessionChangesDisabled(bool value)
    {
        s_FontSessionChangesDisabled = value;
    }
#endif

    namespace
    {
        constexpr std::wstring_view s_FontsPathSubkey = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
        constexpr std::wstring_view s_FontsUserInstallFolder = L"Microsoft\\Windows\\Fonts";
        constexpr std::wstring_view s_TrueType = L" (TrueType)";
        constexpr std::wstring_view s_IdentifierPrefix = L"FONT";
        constexpr std::wstring_view s_Separator = L"\\";
        constexpr std::wstring_view s_FontsWinGetPrefix = L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe";
        const int s_RemoveFontResourceMaxTries = 1000;

        Registry::Key GetFontRegistryHive(Manifest::ScopeEnum scope)
        {
#ifndef AICLI_DISABLE_TEST_HOOKS
            if (s_GetFontRegistryHiveOverride)
            {
                return s_GetFontRegistryHiveOverride(scope);
            }
#endif

            switch (scope)
            {
            case Manifest::ScopeEnum::User:
                return Registry::Key(HKEY_CURRENT_USER, L"", 0UL, MAXIMUM_ALLOWED);
            case Manifest::ScopeEnum::Machine:
                return Registry::Key(HKEY_LOCAL_MACHINE, L"", 0UL, MAXIMUM_ALLOWED);
            case Manifest::ScopeEnum::Unknown:
            default:
                THROW_HR(E_INVALIDARG);
            }
        }

        bool FontPathsEqual(const std::filesystem::path& left, const std::filesystem::path& right)
        {
            return CaseInsensitiveEquals(left.lexically_normal().wstring(), right.lexically_normal().wstring());
        }

        std::wstring GetFontFileTitle(const std::filesystem::path& fontFilePath)
        {
            // This code can fail in a number of ways, including if the file isn't actually
            // a font file or does not have a title.
            wil::com_ptr<IPropertyStore> pPropertyStore;
            if (FAILED(SHGetPropertyStoreFromParsingName(fontFilePath.c_str(), nullptr, GPS_DEFAULT, IID_PPV_ARGS(&pPropertyStore))))
            {
                // Intentionally not throwing here as this is an expected failure case for non-font files or files without titles.
                return {};
            }

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

        void AssertPackageInformation(const FontContext& context)
        {
            if (context.PackageId.empty() || context.PackageVersion.empty())
            {
                // This is a programming error if we reach this point where the package identifier cannot be created or derived.
                THROW_HR_MSG(E_UNEXPECTED, "Package Id and Version must be provided and non-empty.");
            }

            // Defense in depth; the package id and version are used to construct both file system and
            // registry paths. These values do not always originate from a manifest that has been validated:
            // an uninstall without a manifest uses the moniker and version of the installed package, and
            // those are read back from the font registry keys by GetInstalledFontPackages. For a per-user
            // scope those keys are writable by the user, so the values are checked here with the same
            // restrictions that manifest validation applies rather than only checking for directory escapes.
            // These values are only ever checked and never altered, as they must continue to match the paths
            // and registry keys that were persisted at install time.
            std::string packageId = ConvertToUTF8(context.PackageId);
            THROW_HR_IF_MSG(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, !Manifest::IsValueSafeForPathConstruction(packageId), "Package id cannot be used to construct a path: %hs", packageId.c_str());

            std::string packageVersion = ConvertToUTF8(context.PackageVersion);
            THROW_HR_IF_MSG(APPINSTALLER_CLI_ERROR_INVALID_MANIFEST, !Manifest::IsValueSafeForPathConstruction(packageVersion), "Package version cannot be used to construct a path: %hs", packageVersion.c_str());
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
                AssertPackageInformation(context);
                path << s_Separator << s_FontsWinGetPrefix << s_Separator << context.PackageId <<
                    s_Separator << context.PackageVersion;
                break;
            default:
                THROW_HR_MSG(E_UNEXPECTED, "Undefined InstallerSource.");
            }

            return path.str();
        }

        std::filesystem::path GetRootFontPath(Manifest::ScopeEnum scope)
        {
            switch (scope)
            {
            case Manifest::ScopeEnum::Machine:
                return Runtime::GetPathTo(Runtime::PathName::FontsMachineInstallLocation);
            case Manifest::ScopeEnum::User:
                return Runtime::GetPathTo(Runtime::PathName::FontsUserInstallLocation);
            case Manifest::ScopeEnum::Unknown:
            default:
                THROW_HR(E_INVALIDARG);
            }
        }

        std::filesystem::path GetUserFontPackagePath(const FontContext& context)
        {
            AssertPackageInformation(context);
            return GetRootFontPath(Manifest::ScopeEnum::User) / s_FontsWinGetPrefix / context.PackageId / context.PackageVersion;
        }

        void AssertFontFilePath(const FontContext& context, const std::filesystem::path& path)
        {
            const bool inRoot = FontPathsEqual(path.parent_path(), GetRootFontPath(context.Scope));
            const bool inPackage = context.Scope == Manifest::ScopeEnum::User &&
                FontPathsEqual(path.parent_path(), GetUserFontPackagePath(context));
            THROW_HR_IF_MSG(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED,
                !path.is_absolute() || !Manifest::IsValueSafeForPathConstruction(path.filename().u8string()) || (!inRoot && !inPackage),
                "Font path is outside the package's font locations: %ls", path.c_str());
        }

        struct PackageFontFile
        {
            std::wstring RegistryName;
            std::filesystem::path OriginalFileName;
            std::filesystem::path FilePath;
        };

        std::vector<PackageFontFile> GetPackageFontFiles(const FontContext& context)
        {
            auto hive = GetFontRegistryHive(context.Scope);
            auto key = Registry::Key::OpenIfExists(hive, GetFontRegistryPath(context));
            std::vector<PackageFontFile> result;
            if (!key)
            {
                return result;
            }
            for (const auto& entry : key.Values())
            {
                auto value = entry.Value();
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED,
                    !value || value->GetType() != Registry::Value::Type::String);

                PackageFontFile file;
                file.RegistryName = ConvertToUTF16(entry.Name());
                file.FilePath = value->GetValue<Registry::Value::Type::UTF16String>();
                AssertFontFilePath(context, file.FilePath);
                file.OriginalFileName = context.Scope == Manifest::ScopeEnum::User &&
                    FontPathsEqual(file.FilePath.parent_path(), GetUserFontPackagePath(context)) ?
                    file.FilePath.filename() : std::filesystem::path(file.RegistryName);
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED,
                    file.OriginalFileName.empty() || file.OriginalFileName != file.OriginalFileName.filename() ||
                    !Manifest::IsValueSafeForPathConstruction(file.OriginalFileName.u8string()));
                result.push_back(std::move(file));
            }
            return result;
        }

        // Files and registry value names share the scope's font namespace.
        std::filesystem::path GetUniquePathForDestination(const std::filesystem::path& source, Manifest::ScopeEnum scope)
        {
            const auto destination = GetRootFontPath(scope);
            auto hive = GetFontRegistryHive(scope);
            auto root = Registry::Key::OpenIfExists(hive, std::wstring{ s_FontsPathSubkey });
            const auto stem = source.stem();
            const auto ext = source.extension();
            auto uniqueName = source.filename();
            auto candidatePath = destination / uniqueName;
            int index = 0;
            while (std::filesystem::exists(candidatePath) || (root && root[uniqueName.wstring()].has_value()))
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

        std::optional<std::filesystem::path> GetRegisteredFontInstallPath(const FontContext& context, const std::filesystem::path& filePath)
        {
            const auto files = GetPackageFontFiles(context);
            for (const auto& file : files)
            {
                if (FontPathsEqual(file.FilePath, filePath))
                {
                    return file.FilePath;
                }
            }
            for (const auto& file : files)
            {
                if (FontPathsEqual(file.OriginalFileName, filePath.filename()))
                {
                    return file.FilePath;
                }
            }
            return std::nullopt;
        }

        using FontRegistryEntry = std::pair<std::wstring, std::wstring>;

        std::vector<FontRegistryEntry> GetFontRegistrations(Manifest::ScopeEnum scope, const std::filesystem::path& filePath)
        {
            auto hive = GetFontRegistryHive(scope);
            auto root = Registry::Key::OpenIfExists(hive, std::wstring{ s_FontsPathSubkey });
            std::vector<FontRegistryEntry> result;
            if (!root)
            {
                return result;
            }
            const auto rootPath = GetRootFontPath(scope);
            for (const auto& entry : root.Values())
            {
                auto value = entry.Value();
                if (!value || value->GetType() != Registry::Value::Type::String)
                {
                    continue;
                }

                auto data = value->GetValue<Registry::Value::Type::UTF16String>();
                std::filesystem::path registeredPath = data;
                if (registeredPath.is_relative())
                {
                    registeredPath = rootPath / registeredPath;
                }
                if (FontPathsEqual(registeredPath, filePath))
                {
                    result.emplace_back(ConvertToUTF16(entry.Name()), std::move(data));
                }
            }
            return result;
        }

        void UnregisterFontFile(Manifest::ScopeEnum scope, const std::filesystem::path& filePath)
        {
            auto hive = GetFontRegistryHive(scope);
            auto root = Registry::Key::OpenIfExists(hive, std::wstring{ s_FontsPathSubkey }, 0UL, KEY_ALL_ACCESS);
            for (const auto& entry : GetFontRegistrations(scope, filePath))
            {
                root.DeleteValue(entry.first);
            }
        }

        void RegisterFontFile(const FontContext& context, const std::filesystem::path& originalFileName, const std::filesystem::path& installPath)
        {
            AssertFontFilePath(context, installPath);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED,
                !FontPathsEqual(installPath.parent_path(), GetRootFontPath(context.Scope)));
            auto hive = GetFontRegistryHive(context.Scope);
            auto root = Registry::Key::Create(hive, std::wstring{ s_FontsPathSubkey });
            const auto valueName = installPath.filename().wstring();
            if (root[valueName])
            {
                const auto registrations = GetFontRegistrations(context.Scope, installPath);
                THROW_HR_IF_MSG(APPINSTALLER_CLI_ERROR_FONT_INSTALL_FAILED,
                    std::none_of(registrations.begin(), registrations.end(), [&](const auto& entry)
                        { return CaseInsensitiveEquals(entry.first, valueName); }),
                    "Font registration is owned by another file: %ls", valueName.c_str());
            }

            auto package = Registry::Key::Create(hive, GetFontRegistryPath(context));
            const auto originalName = originalFileName.wstring();
            if (const auto existing = package[originalName])
            {
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED, existing->GetType() != Registry::Value::Type::String);
                const std::filesystem::path existingPath = existing->GetValue<Registry::Value::Type::UTF16String>();
                const bool packageFile = context.Scope == Manifest::ScopeEnum::User &&
                    FontPathsEqual(existingPath.parent_path(), GetUserFontPackagePath(context)) &&
                    FontPathsEqual(existingPath.filename(), originalFileName);
                THROW_HR_IF_MSG(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED,
                    !FontPathsEqual(existingPath, installPath) && !packageFile,
                    "Font package mapping is owned by another file: %ls", originalName.c_str());
            }
            package.SetValue(originalName, installPath.wstring());
            root.SetValue(valueName, context.Scope == Manifest::ScopeEnum::Machine ?
                installPath.filename().wstring() : installPath.wstring());
        }

        void AddFontToSession(const std::filesystem::path& path)
        {
#ifndef AICLI_DISABLE_TEST_HOOKS
            if (s_FontSessionChangesDisabled)
            {
                return;
            }
#endif
            const auto fontsAdded = ::AddFontResource(path.c_str());
            if (fontsAdded == 0)
            {
                AICLI_LOG(Core, Warning, << "Failed to add font resource: " << path);
            }
            else
            {
                AICLI_LOG(Core, Info, << "Added " << fontsAdded << " fonts to the session.");
            }
        }

        void RemoveEmptyUserFontPackageDirectories(const FontContext& context)
        {
            if (context.Scope != Manifest::ScopeEnum::User)
            {
                return;
            }

            auto path = GetUserFontPackagePath(context);
            for (size_t i = 0; i < 3; ++i)
            {
                if (std::filesystem::exists(path))
                {
                    if (!std::filesystem::is_empty(path))
                    {
                        break;
                    }
                    std::filesystem::remove(path);
                }
                path = path.parent_path();
            }
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
#ifndef AICLI_DISABLE_TEST_HOOKS
            if (s_FontSessionChangesDisabled)
            {
                return;
            }
#endif
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
#ifndef AICLI_DISABLE_TEST_HOOKS
            if (s_FontSessionChangesDisabled)
            {
                return;
            }
#endif
            // Send the WM_FONTCHANGE message so the system and apps know that there has been a font change.
            // Sometimes this does not have an error when it returns non-zero. To avoid random assert failures
            // we will not try to log failures for this call. If it fails it does not affect install state.
            auto sendResult = ::SendNotifyMessage(HWND_BROADCAST, WM_FONTCHANGE, 0, 0);
            AICLI_LOG(Core, Info, << "Notified system of font change: " << sendResult);
        }

        void MigrateFontFile(const FontContext& context, const PackageFontFile& file)
        {
            auto hive = GetFontRegistryHive(context.Scope);
            auto package = Registry::Key::OpenIfExists(hive, GetFontRegistryPath(context), 0UL, KEY_ALL_ACCESS);
            const auto destination = GetUniquePathForDestination(file.OriginalFileName, context.Scope);
            const auto registrations = GetFontRegistrations(context.Scope, file.FilePath);
            const auto originalName = file.OriginalFileName.wstring();
            const auto originalMapping = package[originalName];
            bool sourceRemoved = false;
            bool sourceUnloaded = false;
            bool mappingChanged = false;
            bool destinationCreated = false;
            auto rollback = wil::scope_exit([&]()
                {
                    if (!destinationCreated)
                    {
                        return;
                    }
                    try
                    {
                        if (sourceRemoved)
                        {
                            std::filesystem::copy_file(destination, file.FilePath);
                        }
                        if (mappingChanged)
                        {
                            if (originalMapping)
                            {
                                package.SetValue(originalName, originalMapping->GetValue<Registry::Value::Type::UTF16String>());
                            }
                            else
                            {
                                package.DeleteValue(originalName);
                            }
                            package.SetValue(file.RegistryName, file.FilePath.wstring());
                        }
                        UnregisterFontFile(context.Scope, destination);
                        auto root = Registry::Key::Create(hive, std::wstring{ s_FontsPathSubkey });
                        for (const auto& registration : registrations)
                        {
                            root.SetValue(registration.first, registration.second);
                        }
                        std::filesystem::remove(destination);
                        if (sourceUnloaded)
                        {
                            AddFontToSession(file.FilePath);
                        }
                    }
                    catch (...)
                    {
                        LOG_CAUGHT_EXCEPTION();
                        AICLI_LOG(Core, Error, << "Font repair rollback failed for " << file.FilePath);
                    }
                });

            std::filesystem::copy_file(file.FilePath, destination);
            destinationCreated = true;
            mappingChanged = true;
            RegisterFontFile(context, file.OriginalFileName, destination);
            UnregisterFontFile(context.Scope, file.FilePath);
            RemoveAllFontResources(file.FilePath);
            sourceUnloaded = true;
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_EXEC_REPAIR_FAILED, !std::filesystem::remove(file.FilePath));
            sourceRemoved = true;
            if (!CaseInsensitiveEquals(file.RegistryName, originalName))
            {
                package.DeleteValue(file.RegistryName);
            }
            rollback.release();
            AddFontToSession(destination);
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
        PackageFiles.push_back(filePath);
    }

    Manifest::AppsAndFeaturesEntry FontContext::GetAppsAndFeaturesEntry() const
    {
        Manifest::AppsAndFeaturesEntry entry;
        entry.DisplayName = ConvertToUTF8(PackageId);
        entry.DisplayVersion = ConvertToUTF8(PackageVersion);
        entry.InstallerType = Manifest::InstallerTypeEnum::Font;

        // Product Code is the PackageId
        entry.ProductCode = ConvertToUTF8(PackageId);
        return entry;
    }

    std::wstring FontContext::GetPackageIdentifier() const
    {
        auto stream = std::wstringstream();
        stream << s_IdentifierPrefix << s_Separator << ConvertToUTF16(Manifest::ScopeToString(Scope)) << s_Separator << PackageId;
        return stream.str();
    }

    // Creates font file info used for install, validation, and enumeration.
    FontFileInfo CreateFontFileInfo(const FontContext& context, const std::filesystem::path& filePath, const std::wstring& title)
    {
        auto fileInfo = FontFileInfo();
        fileInfo.FilePath = filePath;
        fileInfo.PackageIdentifier = context.GetPackageIdentifier();
        fileInfo.PackageId = context.PackageId;
        fileInfo.PackageVersion = context.PackageVersion;
        fileInfo.InstallerSource = context.InstallerSource;
        fileInfo.Scope = context.Scope;
        fileInfo.Title = title;

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

        if (fileInfo.Title.empty())
        {
            // If the title is still empty (such as failure to set it from the File), use the Filename
            fileInfo.Title = fileInfo.FilePath.filename().wstring();
        }

        fileInfo.RegistryPackagePath = GetFontRegistryPath(context);
        fileInfo.RegistryInstallPath = s_FontsPathSubkey;

        const auto registeredPath = context.InstallerSource == InstallerSource::WinGet ?
            GetRegisteredFontInstallPath(context, fileInfo.FilePath) : std::nullopt;
        if (registeredPath)
        {
            fileInfo.InstallPath = registeredPath.value();
        }
        else if (context.InstallerSource == InstallerSource::Unknown)
        {
            fileInfo.InstallPath = fileInfo.FilePath;
        }
        else
        {
            fileInfo.InstallPath = GetUniquePathForDestination(fileInfo.FilePath, context.Scope);
        }

        fileInfo.IsFontFileInstalled = std::filesystem::exists(fileInfo.InstallPath);
        fileInfo.IsFontFileRegistered = !GetFontRegistrations(fileInfo.Scope, fileInfo.InstallPath).empty();
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
        AssertPackageInformation(context);

        auto result = FontValidationResult();

        AICLI_LOG(Core, Info, << "Validating font package: " << context.PackageId.c_str() << " " << context.PackageVersion.c_str());

        try
        {
            // Create font file info for each package file
            for (const auto& file : context.PackageFiles)
            {
                THROW_HR_IF_MSG(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED,
                    std::any_of(result.FontFileInfos.begin(), result.FontFileInfos.end(), [&](const auto& info)
                        { return FontPathsEqual(info.FilePath.filename(), file.filename()); }),
                    "Font package contains duplicate filenames: %ls", file.filename().c_str());
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
        AssertPackageInformation(context);

        FontOperationResult result;

        if (context.InstallerSource != InstallerSource::WinGet)
        {
            throw std::logic_error("Only WinGet format of font install is supported.");
        }

        if (context.PackageFiles.size() == 0)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_FOUND);
            AICLI_LOG(Core, Error, << "Font package has no files: " << context.PackageId.c_str() << " - " << AppInstaller::Logging::SetHRFormat << result.HResult);
            return result;
        }

        // Validate the package data was all processed successfully.
        auto validationResult = ValidateFontPackage(context);
        if (validationResult.Result != FontResult::Success)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
            AICLI_LOG(Core, Error, << "Font package validation failed: " << context.PackageId.c_str() << " - " << AppInstaller::Logging::SetHRFormat << validationResult.HResult);
            return result;
        }

        // Check for unsupported font files.
        if (validationResult.HasUnsupportedFonts)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED);
            AICLI_LOG(Core, Error, << "Font package has unsupported fonts: " << context.PackageId.c_str() << " - " << result.HResult);
            return result;
        }

        // Check for package already installed and correct unless this is a force install.
        if (validationResult.Status == FontStatus::OK && !context.Force)
        {
            result.HResult = winrt::hresult(APPINSTALLER_CLI_ERROR_FONT_ALREADY_INSTALLED);
            AICLI_LOG(Core, Info, << "Font package is already installed and in a good state.: " << context.PackageId.c_str() << " - " << AppInstaller::Logging::SetHRFormat << result.HResult);
            return result;
        }

        const auto installRoot = GetRootFontPath(context.Scope);
        const bool usesPackageDirectory = std::any_of(validationResult.FontFileInfos.begin(), validationResult.FontFileInfos.end(),
            [&](const auto& info) { return !FontPathsEqual(info.InstallPath.parent_path(), installRoot); });
        if (validationResult.Status == FontStatus::Corrupt || context.Force || usesPackageDirectory)
        {
            // Force install of a font can have problems because if the font is already there then it
            // may be in use. The scenarios for using force is if a prior install attempt failed, so
            // we will try to clean up the existing font registration so it may be successful this time.
            AICLI_LOG(Core, Info, << "Removing existing font package mappings before installation.");
            auto uninstallResult = UninstallFontPackage(context);
            if (uninstallResult.Result() != FontResult::Success)
            {
                result.HResult = uninstallResult.HResult;
                AICLI_LOG(Core, Error, << "Font cleanup uninstall failed: " << context.PackageId.c_str() << " - " << AppInstaller::Logging::SetHRFormat << uninstallResult.HResult);
                return result;
            }

            validationResult = ValidateFontPackage(context);
            if (validationResult.Result != FontResult::Success || validationResult.HasUnsupportedFonts)
            {
                result.HResult = APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED;
                AICLI_LOG(Core, Error, << "Font validation failed after cleanup: " << context.PackageId.c_str());
                return result;
            }
        }

        AICLI_LOG(Core, Info, << "Starting install of " << context.PackageId.c_str());

        try
        {
            // Install each file from the file info.
            for (const auto& fontFileInfo : validationResult.FontFileInfos)
            {
                AICLI_LOG(Core, Info, << "Moving " << fontFileInfo.FilePath << " to " << fontFileInfo.InstallPath);
                if (!std::filesystem::exists(fontFileInfo.InstallPath.parent_path()))
                {
                    std::filesystem::create_directories(fontFileInfo.InstallPath.parent_path());
                }

                // Copying into the Fonts directory makes the file visible to system font discovery.
                std::filesystem::copy_file(fontFileInfo.FilePath, fontFileInfo.InstallPath);
                auto removeUnregisteredFile = wil::scope_exit([&]()
                    {
                        try
                        {
                            std::filesystem::remove(fontFileInfo.InstallPath);
                        }
                        catch (...)
                        {
                            LOG_CAUGHT_EXCEPTION();
                            AICLI_LOG(Core, Error, << "Failed removing font file after installation failure: " << fontFileInfo.InstallPath);
                        }
                    });

                AICLI_LOG(Core, Info, << "Adding " << fontFileInfo.Title.c_str() << " to " << fontFileInfo.InstallPath);
                RegisterFontFile(context, fontFileInfo.FilePath.filename(), fontFileInfo.InstallPath);
                std::filesystem::remove(fontFileInfo.FilePath);
                removeUnregisteredFile.release();
                AddFontToSession(fontFileInfo.InstallPath);
            }

            result.HResult = S_OK;
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << "Install failed for " << context.PackageId.c_str() << ", attempting rollback.");

            try
            {
                // Rollback. In this case, rollback is uninstall, which should remove any partial installation.
                // This is best-effort.
                auto rollbackResult = UninstallFontPackage(context);
                if (rollbackResult.Result() == FontResult::Success)
                {
                    AICLI_LOG(Core, Info, << "Rollback for " << context.PackageId.c_str() << " successful.");
                }
                else
                {
                    AICLI_LOG(Core, Error, << "Rollback for " << context.PackageId.c_str() << " failed: " << AppInstaller::Logging::SetHRFormat << rollbackResult.HResult);
                }
            }
            CATCH_LOG();

            result.HResult = APPINSTALLER_CLI_ERROR_FONT_INSTALL_FAILED;
        }

        // Regardless of the result there's likely some changes that occurred.
        NotifyFontChange();
        return result;
    }

    FontOperationResult RepairFontPackage(FontContext& context)
    {
        AssertPackageInformation(context);
        THROW_HR_IF(E_INVALIDARG, context.InstallerSource != InstallerSource::WinGet);
        FontOperationResult result;

        try
        {
            auto files = GetPackageFontFiles(context);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_NO_REPAIR_INFO_FOUND, files.empty());

            FontCatalog catalog;
            for (size_t i = 0; i < files.size(); ++i)
            {
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_FOUND, !std::filesystem::is_regular_file(files[i].FilePath));
                DWRITE_FONT_FILE_TYPE fileType;
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_FILE_NOT_SUPPORTED, !catalog.IsFontFileSupported(files[i].FilePath, fileType));
                for (size_t j = 0; j < i; ++j)
                {
                    THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_VALIDATION_FAILED,
                        FontPathsEqual(files[i].FilePath, files[j].FilePath) ||
                        FontPathsEqual(files[i].OriginalFileName, files[j].OriginalFileName));
                }
            }

            const auto rootPath = GetRootFontPath(context.Scope);
            for (const auto& file : files)
            {
                if (FontPathsEqual(file.FilePath.parent_path(), rootPath))
                {
                    RegisterFontFile(context, file.OriginalFileName, file.FilePath);
                    RemoveAllFontResources(file.FilePath);
                    AddFontToSession(file.FilePath);
                    continue;
                }

                MigrateFontFile(context, file);
            }

            RemoveEmptyUserFontPackageDirectories(context);
        }
        catch (...)
        {
            result.HResult = wil::ResultFromCaughtException();
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << "Font repair failed for " << context.PackageId.c_str());
        }

        NotifyFontChange();
        return result;
    }

    FontOperationResult UninstallFontPackage(FontContext& context)
    {
        AssertPackageInformation(context);
        THROW_HR_IF(E_INVALIDARG, context.InstallerSource != InstallerSource::WinGet);
        FontOperationResult result;

        std::vector<PackageFontFile> files;
        try
        {
            files = GetPackageFontFiles(context);
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << "Failed reading font package mappings.");
            result.HResult = APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED;
            return result;
        }

        auto hive = GetFontRegistryHive(context.Scope);
        auto package = Registry::Key::OpenIfExists(hive, GetFontRegistryPath(context), 0UL, KEY_ALL_ACCESS);
        for (const auto& file : files)
        {
            try
            {
                RemoveAllFontResources(file.FilePath);
                if (std::filesystem::exists(file.FilePath))
                {
                    THROW_HR_IF(APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED, !std::filesystem::remove(file.FilePath));
                }
                UnregisterFontFile(context.Scope, file.FilePath);
                package.DeleteValue(file.RegistryName);
            }
            catch (...)
            {
                LOG_CAUGHT_EXCEPTION();
                AICLI_LOG(Core, Error, << "Failed removing font file or registration: " << file.FilePath);
                result.HResult = APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED;
            }
        }

        if (result.Result() == FontResult::Success)
        {
            try
            {
                if (package)
                {
                    Registry::Key::DeleteTree(hive, GetFontRegistryPath(context));
                }
                RemoveEmptyUserFontPackageDirectories(context);
            }
            catch (...)
            {
                LOG_CAUGHT_EXCEPTION();
                AICLI_LOG(Core, Error, << "Failed removing font package metadata or directories.");
                result.HResult = APPINSTALLER_CLI_ERROR_FONT_UNINSTALL_FAILED;
            }
        }

        // Notify system of font changes.
        NotifyFontChange();
        return result;
    }

    std::wstring GetFontRegistryRoot()
    {
        return s_FontsPathSubkey.data();
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
            // Iterate through scopes for machine and user
            for (const auto& scope : { Manifest::ScopeEnum::Machine, Manifest::ScopeEnum::User })
            {
                std::vector<std::filesystem::path> wingetFonts;
                auto hive = GetFontRegistryHive(scope);
                auto root = Registry::Key::OpenIfExists(hive, std::wstring{ s_FontsPathSubkey });
                if (!root)
                {
                    continue;
                }

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
                            // All sub-keys should be versions of the package.
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
                                    wingetFonts.push_back(filePath);

                                    auto context = FontContext();
                                    context.Scope = scope;
                                    context.InstallerSource = InstallerSource::WinGet;
                                    context.PackageId = packageId;
                                    context.PackageVersion = version;
                                    auto fontFile = CreateFontFileInfo(context, filePath);
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
                    auto registeredPath = filePath.is_relative() ? GetRootFontPath(scope) / filePath : filePath;
                    if (std::any_of(wingetFonts.begin(), wingetFonts.end(),
                        [&](const auto& path) { return FontPathsEqual(path, registeredPath); }))
                    {
                        continue;
                    }

                    auto context = FontContext();
                    context.Scope = scope;
                    context.InstallerSource = InstallerSource::Unknown;
                    context.PackageId = filePath.filename();
                    auto fontFile = CreateFontFileInfo(context, filePath, ConvertToUTF16(rootValue.Name()));
                    fontFiles.push_back(std::move(fontFile));
                }
            }
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << "Failed getting font file information.");
        }

        return fontFiles;
    }

    std::vector<FontPackageInfo> GetInstalledFontPackages(Manifest::ScopeEnum scope)
    {
        auto fontPackages = std::vector<FontPackageInfo>();

        try
        {
            auto hive = GetFontRegistryHive(scope);
            auto root = Registry::Key::OpenIfExists(hive, s_FontsPathSubkey.data(), 0UL, KEY_READ);
            if (!root)
            {
                return fontPackages;
            }

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
                            // All sub-keys should be versions of the package.
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
                                    packageInfo.PackageIdentifier = context.GetPackageIdentifier();
                                    fontPackages.push_back(std::move(packageInfo));
                                }
                            }
                        }
                        catch (...)
                        {
                            // If we have bad data in the registry for this entry, log it and skip.
                            LOG_CAUGHT_EXCEPTION();
                            AICLI_LOG(Core, Error, << "Failed getting font package information for: " << packageSubKey.Name().c_str());
                            continue;
                        }
                    }
                }
            }
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            AICLI_LOG(Core, Error, << "Failed getting font package information.");
        }

        return fontPackages;
    }
}
