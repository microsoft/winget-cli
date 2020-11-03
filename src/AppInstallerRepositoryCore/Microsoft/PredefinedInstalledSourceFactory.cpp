// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Microsoft/SQLiteIndex.h"
#include "Microsoft/SQLiteIndexSource.h"
#include <winget/ManifestInstaller.h>

#include <winget/Registry.h>
#include <AppInstallerArchitecture.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // A helper to find the various locations that contain ARP entries.
        struct ARPHelper
        {
            // See https://docs.microsoft.com/en-us/windows/win32/msi/uninstall-registry-key for details.
            std::wstring SubKeyPath{ L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall" };

            // REG_SZ
            std::wstring DisplayName{ L"DisplayName" };
            // REG_SZ
            std::wstring Publisher{ L"Publisher" };
            // REG_SZ
            std::wstring DisplayVersion{ L"DisplayVersion" };
            // REG_DWORD (ex. 0xMMmmbbbb, M[ajor], m[inor], b[uild])
            std::wstring Version{ L"Version" };
            // REG_DWORD
            std::wstring VersionMajor{ L"VersionMajor" };
            // REG_DWORD
            std::wstring VersionMinor{ L"VersionMinor" };
            // REG_SZ
            std::wstring URLInfoAbout{ L"URLInfoAbout" };
            // REG_SZ
            std::wstring HelpLink{ L"HelpLink" };
            // REG_SZ
            std::wstring InstallLocation{ L"InstallLocation" };
            // REG_DWORD (ex. 1033 [en-us])
            std::wstring Language{ L"Language" };
            // REG_SZ (ex. "english")
            std::wstring InnoSetupLanguage{ L"Inno Setup: Language" };
            // REG_EXPAND_SZ
            std::wstring UninstallString{ L"UninstallString" };
            // REG_EXPAND_SZ
            std::wstring QuietUninstallString{ L"QuietUninstallString" };
            // REG_DWORD (bool, true indicates MSI)
            std::wstring WindowsInstaller{ L"WindowsInstaller" };
            // REG_DWORD (bool)
            std::wstring SystemComponent{ L"SystemComponent" };

            Registry::Key GetARPForArchitecture(Manifest::ManifestInstaller::ScopeEnum scope, Utility::Architecture architecture)
            {
                HKEY rootKey = NULL;

                switch (scope)
                {
                case Manifest::ManifestInstaller::ScopeEnum::User:
                    rootKey = HKEY_CURRENT_USER;
                    break;
                case Manifest::ManifestInstaller::ScopeEnum::Machine:
                    rootKey = HKEY_LOCAL_MACHINE;
                    break;
                default:
                    THROW_HR(E_UNEXPECTED);
                }

                bool isValid = false;
                REGSAM access = KEY_READ;

                switch (Utility::GetSystemArchitecture())
                {
                case Utility::Architecture::X86:
                    switch (architecture)
                    {
                    case Utility::Architecture::X86:
                        isValid = true;
                        break;
                    }
                    break;
                case Utility::Architecture::X64:
                    switch (architecture)
                    {
                    case Utility::Architecture::X86:
                        if (scope == Manifest::ManifestInstaller::ScopeEnum::Machine)
                        {
                            access |= KEY_WOW64_32KEY;
                            isValid = true;
                        }
                        break;
                    case Utility::Architecture::X64:
                        access |= KEY_WOW64_64KEY;
                        isValid = true;
                        break;
                    }
                    break;
                case Utility::Architecture::Arm:
                    switch (architecture)
                    {
                    case Utility::Architecture::Arm:
                        isValid = true;
                        break;
                    }
                    break;
                case Utility::Architecture::Arm64:
                    switch (architecture)
                    {
                    case Utility::Architecture::X86:
                        if (scope == Manifest::ManifestInstaller::ScopeEnum::Machine)
                        {
#ifdef _ARM_
                            // Not accessible if this is an ARM process
                            AICLI_LOG(Repo, Warning, << "Cannot enumerate x86 ARP entries currently");
#else
                            access |= KEY_WOW64_32KEY;
                            isValid = true;
#endif
                        }
                        break;
                    case Utility::Architecture::Arm64:
                        access |= KEY_WOW64_64KEY;
                        isValid = true;
                        break;
                    }
                    break;
                }

                if (isValid)
                {
                    return Registry::Key::OpenIfExists(rootKey, SubKeyPath, 0, access);
                }
                else
                {
                    return {};
                }
            }

            // Returns true IFF the value exists and contains a non-zero DWORD.
            bool GetBoolValue(const Registry::Key& arpKey, const std::wstring& name)
            {
                auto value = arpKey[name];
                return (value && value->GetType() == Registry::Value::Type::DWord && value->GetValue<Registry::Value::Type::DWord>());
            }

            // Determines the version from an ARP entry.
            // The priority is:
            //  DisplayVersion
            //  Version
            //  MajorVerison, MinorVersion
            std::string DetermineVersion(const Registry::Key& arpKey)
            {
                auto displayVersion = arpKey[DisplayVersion];
                if (displayVersion)
                {
                    std::string result = displayVersion->GetValue<Registry::Value::Type::String>();
                    if (!result.empty())
                    {
                        return result;
                    }
                }

                auto version = arpKey[Version];
                if (version)
                {
                    uint32_t versionInt = version->GetValue<Registry::Value::Type::DWord>();
                    if (versionInt)
                    {
                        std::ostringstream strstr;
                        strstr << ((versionInt & 0xFF000000) >> 24) << '.' << ((versionInt & 0x00FF0000) >> 16) << '.' << (versionInt & 0x0000FFFF);
                        return strstr.str();
                    }
                }

                auto majorVersion = arpKey[VersionMajor];
                auto minorVersion = arpKey[VersionMinor];
                if (majorVersion || minorVersion)
                {
                    uint32_t majorVersionInt = 0;
                    uint32_t minorVersionInt = 0;

                    if (majorVersion)
                    {
                        majorVersionInt = majorVersion->GetValue<Registry::Value::Type::DWord>();
                    }

                    if (minorVersion)
                    {
                        minorVersionInt = minorVersion->GetValue<Registry::Value::Type::DWord>();
                    }

                    if (majorVersionInt || minorVersionInt)
                    {
                        std::ostringstream strstr;
                        strstr << majorVersionInt << '.' << minorVersionInt;
                        return strstr.str();
                    }
                }

                return {};
            }

            static void AddMetadataIfPresent(const Registry::Key& key, const std::wstring& name, SQLiteIndex& index, SQLiteIndex::IdType manifestId, PackageVersionMetadata metadata)
            {
                auto value = key[name];
                if (value)
                {
                    auto valueString = value->GetValue<Registry::Value::Type::String>();
                    if (!valueString.empty())
                    {
                        index.SetMetadataByManifestId(manifestId, metadata, valueString);
                    }
                }
            }

            // Populates the index with the ARP entries from the given root.
            void PopulateIndexFromARP(SQLiteIndex& index, Manifest::ManifestInstaller::ScopeEnum scope)
            {
                std::string_view scopeString{ Manifest::ManifestInstaller::ScopeToString(scope) };

                for (auto architecture : Utility::GetApplicableArchitectures())
                {
                    Registry::Key arpRootKey = GetARPForArchitecture(scope, architecture);

                    if (arpRootKey)
                    {
                        std::string_view architectureString = Utility::ToString(architecture);

                        AICLI_LOG(Repo, Info, << "Examining ARP entries for " << scopeString << " | " << architectureString);

                        for (const auto& arpEntry : arpRootKey)
                        {
                            Manifest::Manifest manifest;
                            manifest.Tags = { "ARP" };

                            // Use the key name as the Id, as it is supposed to be unique.
                            // TODO: We probably want something better here, like constructing the value as
                            //       `Publisher.DisplayName`. We would need to ensure that there are no matches
                            //       against the rest of the data however (might happen if same package is
                            //       installed for multiple architectures/languages).
                            manifest.Id = arpEntry.Name();

                            manifest.Installers.emplace_back();
                            // TODO: This likely needs some cleanup applied, as it looks like INNO tends to append an "_is#"
                            //       that might vary across machines/installs. There may be other things we want to clean up as well,
                            //       like trimming spaces at the ends, or removing the version string from the product code
                            //       if it is present.
                            manifest.Installers[0].ProductCode = arpEntry.Name();
                            manifest.Installers[0].Scope = scope;

                            Registry::Key arpKey = arpEntry.Open();

                            // Ignore entries that are listed as SystemComponent
                            if (GetBoolValue(arpKey, SystemComponent)) { continue; }

                            // If no name is provided, ignore this entry
                            auto displayName = arpKey[DisplayName];
                            if (!displayName) { continue; }
                            manifest.Name = displayName->GetValue<Registry::Value::Type::String>();
                            if (manifest.Name.empty()) { continue; }

                            // If no version can be determined, ignore this entry
                            manifest.Version = DetermineVersion(arpKey);
                            if (manifest.Version.empty()) { continue; }

                            auto publisher = arpKey[Publisher];
                            if (publisher)
                            {
                                manifest.Publisher = publisher->GetValue<Registry::Value::Type::String>();
                            }

                            // TODO: If we want to keep the constructed manifest around to allow for `show` type commands
                            //       against installed packages, we should use URLInfoAbout/HelpLink for the Homepage.

                            // TODO: Pick up Language/InnoSetupLanguage to enable proper selection of language for upgrade.

                            // TODO: Determine the best way to handle duplicates, which may very well happen.
                            //       For now, we will attempt to insert and catch, then send failure telemetry.
                            //       In a future where we cache these entries
                            std::optional<SQLiteIndex::IdType> manifestIdOpt;
                            HRESULT addHr = S_OK;

                            try
                            {
                                // Use the ProductCode as a unique key for the path
                                manifestIdOpt = index.AddManifest(manifest, Utility::ConvertToUTF16(manifest.Installers[0].ProductCode));
                            }
                            catch (wil::ResultException& re)
                            {
                                addHr = re.GetErrorCode();
                            }
                            catch (...)
                            {
                                addHr = E_FAIL;
                            }

                            if (!manifestIdOpt)
                            {
                                Logging::Telemetry().LogDuplicateARPEntry(addHr, scopeString, Utility::ToString(architecture), manifest.Installers[0].ProductCode, manifest.Name);
                                continue;
                            }

                            SQLiteIndex::IdType manifestId = manifestIdOpt.value();

                            // Pass scope along to metadata.
                            index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledScope, scopeString);

                            // TODO: Pass along architecture, although there are cases where it is not clear what architecture the package
                            //       is from it's ARP location, despite it very clearly being a specific architecture. And note that user
                            //       scope does not have separate ARP locations, so every architecture would appear as native.

                            // Pick up InstallLocation when upgrade supports remove/install to enable this location
                            // to survive across the removal.
                            AddMetadataIfPresent(arpKey, InstallLocation, index, manifestId, PackageVersionMetadata::InstalledLocation);

                            // Pick up UninstallString and QuietUninstallString for uninstall.
                            AddMetadataIfPresent(arpKey, UninstallString, index, manifestId, PackageVersionMetadata::StandardUninstallCommand);
                            AddMetadataIfPresent(arpKey, QuietUninstallString, index, manifestId, PackageVersionMetadata::SilentUninstallCommand);

                            // Pick up WindowsInstaller to determine if this is an MSI install.
                            auto installedType = Manifest::ManifestInstaller::InstallerTypeEnum::Exe;

                            if (GetBoolValue(arpKey, WindowsInstaller))
                            {
                                installedType = Manifest::ManifestInstaller::InstallerTypeEnum::Msi;
                            }

                            index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType, Manifest::ManifestInstaller::InstallerTypeToString(installedType));
                        }
                    }
                }
            }
        };

        // Populates the index with the entries from MSIX.
        void PopulateIndexFromMSIX(SQLiteIndex& index)
        {
            using namespace winrt::Windows::ApplicationModel;
            using namespace winrt::Windows::Management::Deployment;

            // TODO: Consider if Optional packages should also be enumerated
            PackageManager packageManager;
            auto packages = packageManager.FindPackagesForUserWithPackageTypes({}, PackageTypes::Main);

            // Reuse the same manifest object, as we will be setting the same values every time.
            Manifest::Manifest manifest;
            // Add one installer for storing the package family name.
            manifest.Installers.emplace_back();
            // Every package will have the same tags currently.
            manifest.Tags = { "msix" };

            // Fields in the index but not populated:
            //  AppMoniker - Not sure what we would put.
            //  Channel - We don't know this information here.
            //  Commands - We could open the manifest and look for these eventually.
            //  Tags - Not sure what else we could put in here.
            for (const auto& package : packages)
            {
                // System packages are part of the OS, and cannot be managed by the user.
                // Filter them out as there is no point in showing them in a package manager.
                auto signatureKind = package.SignatureKind();
                if (signatureKind == PackageSignatureKind::System)
                {
                    continue;
                }

                auto packageId = package.Id();
                Utility::NormalizedString familyName = Utility::ConvertToUTF8(packageId.FamilyName());

                manifest.Id = familyName;
                manifest.Name = Utility::ConvertToUTF8(package.DisplayName());

                if (manifest.Name.empty())
                {
                    manifest.Name = Utility::ConvertToUTF8(packageId.Name());
                }

                std::ostringstream strstr;
                auto packageVersion = packageId.Version();
                strstr << packageVersion.Major << '.' << packageVersion.Minor << '.' << packageVersion.Build << '.' << packageVersion.Revision;

                manifest.Version = strstr.str();
                
                manifest.Installers[0].PackageFamilyName = familyName;

                // Use the family name as a unique key for the path
                auto manifestId = index.AddManifest(manifest, std::filesystem::path{ packageId.FamilyName().c_str() });

                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType, 
                    Manifest::ManifestInstaller::InstallerTypeToString(Manifest::ManifestInstaller::InstallerTypeEnum::Msix));
            }
        }

        // The factory for the predefined installed source.
        struct Factory : public ISourceFactory
        {
            std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback& progress) override final
            {
                // TODO: Maybe we do need to use it?
                UNREFERENCED_PARAMETER(progress);

                THROW_HR_IF(E_INVALIDARG, details.Type != PredefinedInstalledSourceFactory::Type());

                // Determine the filter
                PredefinedInstalledSourceFactory::Filter filter = PredefinedInstalledSourceFactory::StringToFilter(details.Arg);
                AICLI_LOG(Repo, Info, << "Creating PredefinedInstalledSource with filter [" << PredefinedInstalledSourceFactory::FilterToString(filter) << ']');

                // Create an in memory index
                SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, Schema::Version::Latest());

                // Put installed packages into the index
                std::optional<ARPHelper> arpHelper;

                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::ARP_System)
                {
                    if (!arpHelper)
                    {
                        arpHelper = ARPHelper();
                    }

                    arpHelper->PopulateIndexFromARP(index, Manifest::ManifestInstaller::ScopeEnum::Machine);
                }

                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::ARP_User)
                {
                    if (!arpHelper)
                    {
                        arpHelper = ARPHelper();
                    }

                    arpHelper->PopulateIndexFromARP(index, Manifest::ManifestInstaller::ScopeEnum::User);
                }

                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::MSIX)
                {
                    PopulateIndexFromMSIX(index);
                }

                return std::make_shared<SQLiteIndexSource>(details, "*PredefinedInstalledSource", std::move(index), Synchronization::CrossProcessReaderWriteLock{}, true);
            }

            void Add(SourceDetails&, IProgressCallback&) override final
            {
                // Add should never be needed, as this is predefined.
                THROW_HR(E_NOTIMPL);
            }

            void Update(const SourceDetails&, IProgressCallback&) override final
            {
                // Update could be used later, but not for now.
                THROW_HR(E_NOTIMPL);
            }

            void Remove(const SourceDetails&, IProgressCallback&) override final
            {
                // Similar to add, remove should never be needed.
                THROW_HR(E_NOTIMPL);
            }
        };
    }

    std::string_view PredefinedInstalledSourceFactory::FilterToString(Filter filter)
    {
        switch (filter)
        {
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::None:
            return "None"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::ARP_System:
            return "ARP_System"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::ARP_User:
            return "ARP_User"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::MSIX:
            return "MSIX"sv;
        default:
            return "Unknown"sv;
        }
    }

    PredefinedInstalledSourceFactory::Filter PredefinedInstalledSourceFactory::StringToFilter(std::string_view filter)
    {
        if (filter == FilterToString(Filter::ARP_System))
        {
            return Filter::ARP_System;
        }
        else if (filter == FilterToString(Filter::ARP_User))
        {
            return Filter::ARP_User;
        }
        else if (filter == FilterToString(Filter::MSIX))
        {
            return Filter::MSIX;
        }
        else
        {
            return Filter::None;
        }
    }

    std::unique_ptr<ISourceFactory> PredefinedInstalledSourceFactory::Create()
    {
        return std::make_unique<Factory>();
    }
}
