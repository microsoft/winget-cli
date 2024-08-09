// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ARPHelper.h"
#include "winget/PortableARPEntry.h"

namespace AppInstaller::Repository::Microsoft
{
    using namespace AppInstaller::Registry::Portable;

    namespace
    {
        // "Unpacks" a GUID in the format used by the UpgradesCode registry key into the usual format.
        // Returns empty if it is not a valid GUID
        std::optional<std::string> TryUnpackUpgradeCodeGuid(std::string_view packed)
        {
            // A GUID is made up of 4 parts:
            //   - Part 1 is made up of one 4 byte block
            //   - Parts 2 and 3 are made up of one 2 byte block
            //   - Part 4 is made up of eight 1 byte blocks
            //
            // The GUID strings we have in the manifests represent all of this in hex in order,
            // with dashes between each part, and after the second byte of Part 4.
            // The "packed" GUIDs in the registry place the blocks in the same order,
            // without dashes and with opposite endian-ness.
            //
            // For example
            //   ARP:          {FECAFEB5-8D0E-4AE4-8FA0-745BAA835C35}
            //                FECAFEB5 8D0E 4AE4 8F A0 74 5B AA 83 5C 35
            //                 Part 1   P2   P3  <------ Part 4 ------->
            //                5BEFACEF E0D8 4EA4 F8 0A 47 B5 AA 38 C5 53
            //   UpgradeCode:     5BEFACEFE0D84EA4F80A47B5AA38C553
            //
            // The conversion can be done by mapping each location in the packed string
            // to the appropriate location in the unpacked string.
            constexpr size_t PackedLength = 32;
            if (packed.length() != PackedLength || !std::all_of(packed.begin(), packed.end(), isxdigit))
            {
                return {};
            }

            // PositionMapping[i] is the position to which the i-th char is mapped
            // I.e., unpacked[ PositionMapping[i] ] = packed[i]
            constexpr size_t PositionMapping[PackedLength] =
            {
                8,7,6,5,4,3,2,1,
                13,12,11,10,
                18,17,16,15,
                21,20, 23,22,
                26,25, 28,27, 30,29, 32,31, 34,33, 36,35,
            };

            std::string unpacked("{00000000-0000-0000-0000-000000000000}");
            for (size_t i = 0; i < PackedLength; ++i)
            {
                unpacked[PositionMapping[i]] = packed[i];
            }

            return unpacked;
        }

        // Gets a mapping from ProductCode to UpgradeCode for MSI packages.
        std::map<std::string, std::string> GetUpgradeCodes()
        {
            // The UpgradeCode is not stored in the ARP registry keys, so we have to get it separately.
            // We could use MsiGetProductProperty or MsiGetProperty from the MSI API to query it,
            // but it is very slow.
            //
            // The UpgradeCode is also stored in the registry under
            //   HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Installer\UpgradeCodes
            // (Note that this key is not documented, so it is possible that it will change but very unlikely...)
            //
            // Under 'UpgradeCodes' there is one key for each upgrade code, and each upgrade code key
            // contains the product code as a value. All the upgrade codes and product codes are GUIDs,
            // but represented in an unusual way - see TryUnpackUpgradeCodeGuid()

            AICLI_LOG(Repo, Info, << "Reading MSI UpgradeCodes");
            std::map<std::string, std::string> upgradeCodes;

            try
            {
                // There is no UpgradeCodes key on the x86 view of the registry
                Registry::Key upgradeCodesKey = Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Installer\\UpgradeCodes", 0, KEY_READ | KEY_WOW64_64KEY);

                if (upgradeCodesKey)
                {
                    for (const auto& upgradeCodeKeyRef : upgradeCodesKey)
                    {
                        std::string keyName;

                        try
                        {
                            keyName = upgradeCodeKeyRef.Name();
                            auto upgradeCode = TryUnpackUpgradeCodeGuid(keyName);
                            if (upgradeCode)
                            {
                                auto upgradeCodeKey = upgradeCodeKeyRef.Open();
                                for (const auto& productCodeValue : upgradeCodeKey.Values())
                                {
                                    auto productCode = TryUnpackUpgradeCodeGuid(productCodeValue.Name());
                                    if (productCode)
                                    {
                                        upgradeCodes[*productCode] = *upgradeCode;
                                    }
                                }
                            }
                        }
                        CATCH_LOG_MSG("Failed to read upgrade code: %hs", keyName.c_str());
                    }
                }
            }
            CATCH_LOG_MSG("Failed to read upgrade codes.");

            return upgradeCodes;
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    using GetARPKeyFunc = std::function<Registry::Key(Manifest::ScopeEnum, Utility::Architecture)>;
    static GetARPKeyFunc s_GetARPKey_Override;

    void SetGetARPKeyOverride(GetARPKeyFunc value)
    {
        s_GetARPKey_Override = value;
    }
#endif

    Registry::Key ARPHelper::GetARPKey(Manifest::ScopeEnum scope, Utility::Architecture architecture) const
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_GetARPKey_Override)
        {
            return s_GetARPKey_Override(scope, architecture);
        }
#endif

        HKEY rootKey = NULL;

        switch (scope)
        {
        case Manifest::ScopeEnum::User:
            rootKey = HKEY_CURRENT_USER;
            break;
        case Manifest::ScopeEnum::Machine:
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
                if (scope == Manifest::ScopeEnum::Machine)
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
                if (scope == Manifest::ScopeEnum::Machine)
                {
#ifdef _ARM_
                    // Not accessible if this is an ARM process
                    AICLI_LOG(Repo, Warning, << "Cannot enumerate x86 machine ARP entries when current process is ARM");
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

    Registry::Key ARPHelper::FindARPEntry(const std::string& productCode, Manifest::ScopeEnum scope) const
    {
        if (productCode.empty())
        {
            return {};
        }

        std::vector<Manifest::ScopeEnum> scopesToSearch;
        if (scope == Manifest::ScopeEnum::Unknown)
        {
            scopesToSearch = { Manifest::ScopeEnum::User, Manifest::ScopeEnum::Machine };
        }
        else
        {
            scopesToSearch = { scope };
        }

        for (auto scopeToSearch : scopesToSearch)
        {
            for (auto architecture : Utility::GetApplicableArchitectures())
            {
                Registry::Key arpRootKey = GetARPKey(scopeToSearch, architecture);
                if (arpRootKey)
                {
                    for (const auto& entry : arpRootKey)
                    {
                        if (Utility::CaseInsensitiveEquals(productCode, entry.Name()))
                        {
                            return entry.Open();
                        }
                    }
                }
            }
        }

        return {};
    }

    bool ARPHelper::GetBoolValue(const Registry::Key& arpKey, const std::wstring& name)
    {
        auto value = arpKey[name];
        return (value && value->GetType() == Registry::Value::Type::DWord && value->GetValue<Registry::Value::Type::DWord>());
    }

    std::string ARPHelper::GetStringValue(const Registry::Key& arpKey, const std::wstring& name)
    {
        auto value = arpKey[name];
        if (value && value->GetType() == Registry::Value::Type::String)
        {
            return value->GetValue<Registry::Value::Type::String>();
        }

        return {};
    }

    std::string ARPHelper::DetermineVersion(const Registry::Key& arpKey) const
    {
        // First check DisplayVersion for a complete version string
        auto displayVersion = arpKey[DisplayVersion];
        if (displayVersion && displayVersion->GetType() == Registry::Value::Type::String)
        {
            std::string result = displayVersion->GetValue<Registry::Value::Type::String>();
            if (!result.empty())
            {
                return result;
            }
        }

        // Next attempt VersionMajor.VersionMinor, then MajorVersion.MinorVersion
        for (const auto& names : { std::make_pair(std::ref(VersionMajor), std::ref(VersionMinor)), std::make_pair(std::ref(MajorVersion), std::ref(MinorVersion)) })
        {
            auto majorVersion = arpKey[names.first];
            auto minorVersion = arpKey[names.second];
            if (majorVersion || minorVersion)
            {
                uint32_t majorVersionInt = 0;
                uint32_t minorVersionInt = 0;

                if (majorVersion && majorVersion->GetType() == Registry::Value::Type::DWord)
                {
                    majorVersionInt = majorVersion->GetValue<Registry::Value::Type::DWord>();
                }

                if (minorVersion && minorVersion->GetType() == Registry::Value::Type::DWord)
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
        }

        // Finally attempt to turn the Version DWORD into a version string
        auto version = arpKey[Version];
        if (version && version->GetType() == Registry::Value::Type::DWord)
        {
            uint32_t versionInt = version->GetValue<Registry::Value::Type::DWord>();
            if (versionInt)
            {
                std::ostringstream strstr;
                strstr << ((versionInt & 0xFF000000) >> 24) << '.' << ((versionInt & 0x00FF0000) >> 16) << '.' << (versionInt & 0x0000FFFF);
                return strstr.str();
            }
        }

        return Utility::Version::CreateUnknown().ToString();
    }

    void ARPHelper::AddMetadataIfPresent(const Registry::Key& key, const std::wstring& name, SQLiteIndex& index, SQLiteIndex::IdType manifestId, PackageVersionMetadata metadata) const
    {
        auto value = key[name];
        if (value)
        {
            std::string valueString;

            if (value->GetType() == Registry::Value::Type::String)
            {
                valueString = value->GetValue<Registry::Value::Type::String>();
            }
            else if (value->GetType() == Registry::Value::Type::ExpandString)
            {
                valueString = value->GetValue<Registry::Value::Type::ExpandString>();
            }
            else if (value->GetType() == Registry::Value::Type::DWord)
            {
                DWORD dwordValue = value->GetValue<Registry::Value::Type::DWord>();
                if (name == Language)
                {
                    valueString = Locale::LocaleIdToBcp47Tag(dwordValue);
                }
                else
                {
                    std::ostringstream strstr;
                    strstr << dwordValue;
                    valueString = strstr.str();
                }
            }

            if (!valueString.empty())
            {
                index.SetMetadataByManifestId(manifestId, metadata, valueString);
            }
        }
    }

    void ARPHelper::PopulateIndexFromARP(SQLiteIndex& index, Manifest::ScopeEnum scope) const
    {
        auto upgradeCodes = GetUpgradeCodes();

        for (auto architecture : Utility::GetApplicableArchitectures())
        {
            Registry::Key arpRootKey = GetARPKey(scope, architecture);

            if (arpRootKey)
            {
                PopulateIndexFromKey(index, arpRootKey, Manifest::ScopeToString(scope), Utility::ToString(architecture), upgradeCodes);
            }
        }
    }

    void ARPHelper::PopulateIndexFromKey(SQLiteIndex& index, const Registry::Key& key, std::string_view scope, std::string_view architecture, const std::map<std::string, std::string>& upgradeCodes) const
    {
        AICLI_LOG(Repo, Verbose, << "Examining ARP entries for " << scope << " | " << architecture);

        for (const auto& arpEntry : key)
        {
            std::string productCode;

            try
            {
                productCode = arpEntry.Name();

                Manifest::Manifest manifest;
                manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ "ARP" });

                // Construct a unique name for this entry
                const char separator = '\\';

                std::ostringstream stream;
                stream << "ARP" << separator << scope << separator << architecture << separator << productCode;

                manifest.Id = stream.str();

                manifest.Installers.emplace_back();
                // TODO: This likely needs some cleanup applied, as it looks like INNO tends to append an "_is#"
                //       that might vary across machines/installs. There may be other things we want to clean up as well,
                //       like trimming spaces at the ends, or removing the version string from the product code
                //       if it is present.
                manifest.Installers[0].ProductCode = productCode;

                Registry::Key arpKey = arpEntry.Open();

                // Ignore entries that are listed as SystemComponent
                if (GetBoolValue(arpKey, SystemComponent))
                {
                    AICLI_LOG(Repo, Verbose, << "Skipping " << productCode << " because it is a SystemComponent");
                    continue;
                }

                // If no name is provided, ignore this entry
                auto displayName = arpKey[DisplayName];
                if (!displayName || displayName->GetType() != Registry::Value::Type::String)
                {
                    AICLI_LOG(Repo, Verbose, << "Skipping " << productCode << " because DisplayName is not a REG_SZ value");
                    continue;
                }
                auto displayNameValue = displayName->GetValue<Registry::Value::Type::String>();
                if (displayNameValue.empty())
                {
                    AICLI_LOG(Repo, Verbose, << "Skipping " << productCode << " because DisplayName is empty");
                    continue;
                }

                manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(displayNameValue);
                // Add DisplayName to ARP entries too
                // This is to help normalized publisher and name correlation where ARP DisplayName matching
                // will be getting improved in future iterations.
                manifest.Installers[0].AppsAndFeaturesEntries.emplace_back();
                manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayName = displayNameValue;

                // If no version can be determined, ignore this entry
                manifest.Version = DetermineVersion(arpKey);
                if (manifest.Version.empty())
                {
                    AICLI_LOG(Repo, Verbose, << "Skipping " << productCode << " because a version could not be determined");
                    continue;
                }

                auto publisher = arpKey[Publisher];
                if (publisher && publisher->GetType() == Registry::Value::Type::String)
                {
                    manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(publisher->GetValue<Registry::Value::Type::String>());

                    // If Publisher is set, change the Id using name normalization
                    // TODO: Figure out how to actually make this work since there are often instances of the same
                    // data in x64 and x86 entries that will collide.
                    //auto normalizedName = index.NormalizeName(
                    //    manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>(),
                    //    manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
                    //manifest.Id = normalizedName.Publisher() + '.' + normalizedName.Name();
                }

                // Pick up WindowsInstaller to determine if this is an MSI install.
                // TODO: Could also determine Inno (and maybe other types) through detecting other keys here.
                auto installedType = Manifest::InstallerTypeEnum::Exe;

                if (GetBoolValue(arpKey, WindowsInstaller))
                {
                    installedType = Manifest::InstallerTypeEnum::Msi;

                    // If this is an MSI, look up the UpgradeCode
                    auto upgradeCodeItr = upgradeCodes.find(productCode);
                    if (upgradeCodeItr != upgradeCodes.end())
                    {
                        manifest.Installers[0].AppsAndFeaturesEntries[0].UpgradeCode = upgradeCodeItr->second;
                    }
                }

                // TODO: If we want to keep the constructed manifest around to allow for `show` type commands
                //       against installed packages, we should use URLInfoAbout/HelpLink for the Homepage.

                // TODO: Determine the best way to handle duplicates; sometimes the same package will be listed under
                //       both x64 and x86 locations for ARP.
                //       For now, we will attempt to insert and catch.
                std::optional<SQLiteIndex::IdType> manifestIdOpt;

                try
                {
                    // Use the ProductCode as a unique key for the path
                    manifestIdOpt = index.AddManifest(manifest);
                }
                catch (...)
                {
                    // Ignore errors if they occur, they are most likely a duplicate value
                }

                if (!manifestIdOpt)
                {
                    AICLI_LOG(Repo, Warning,
                        << "Ignoring duplicate ARP entry " << scope << '|' << architecture << '|' << productCode << " [" << manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>() << "]");
                    continue;
                }

                SQLiteIndex::IdType manifestId = manifestIdOpt.value();

                // Pass scope along to metadata.
                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledScope, scope);

                // TODO: Pass along architecture, although there are cases where it is not clear what architecture the package
                //       is from it's ARP location, despite it very clearly being a specific architecture. And note that user
                //       scope does not have separate ARP locations, so every architecture would appear as native.

                // Publisher is needed for certain scenarios but we don't store it from the manifest
                if (manifest.DefaultLocalization.Contains(Manifest::Localization::Publisher))
                {
                    index.SetMetadataByManifestId(
                        manifestId, PackageVersionMetadata::Publisher,
                        manifest.DefaultLocalization.Get<Manifest::Localization::Publisher>());
                }

                // Pick up InstallLocation when upgrade supports remove/install to enable this location
                // to survive across the removal.
                AddMetadataIfPresent(arpKey, InstallLocation, index, manifestId, PackageVersionMetadata::InstalledLocation);

                // Pick up UninstallString and QuietUninstallString for uninstall.
                AddMetadataIfPresent(arpKey, UninstallString, index, manifestId, PackageVersionMetadata::StandardUninstallCommand);
                AddMetadataIfPresent(arpKey, QuietUninstallString, index, manifestId, PackageVersionMetadata::SilentUninstallCommand);

                // Pick up ModifyPath for repair.
                AddMetadataIfPresent(arpKey, ModifyPath, index, manifestId, PackageVersionMetadata::StandardModifyCommand);
                AddMetadataIfPresent(arpKey, NoModify, index, manifestId, PackageVersionMetadata::NoModify);
                AddMetadataIfPresent(arpKey, NoRepair, index, manifestId, PackageVersionMetadata::NoRepair);

                // Pick up Language to enable proper selection of language for upgrade.
                AddMetadataIfPresent(arpKey, Language, index, manifestId, PackageVersionMetadata::InstalledLocale);

                if (Manifest::ConvertToInstallerTypeEnum(GetStringValue(arpKey, std::wstring{ ToString(PortableValueName::WinGetInstallerType) })) == Manifest::InstallerTypeEnum::Portable)
                {
                    // Portable uninstall requires the installed architecture for locating the entry in the registry.
                    index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledArchitecture, architecture);
                    installedType = Manifest::InstallerTypeEnum::Portable;
                }

                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType, Manifest::InstallerTypeToString(installedType));
            }
            catch (...)
            {
                AICLI_LOG(Repo, Warning, << "Failed to read ARP entry, ignoring it: " << scope << '|' << architecture << '|' << productCode);
                LOG_CAUGHT_EXCEPTION();
            }
        }
    }

    std::vector<wil::unique_registry_watcher> ARPHelper::CreateRegistryWatchers(Manifest::ScopeEnum scope, std::function<void(Manifest::ScopeEnum, Utility::Architecture, wil::RegistryChangeKind)> callback)
    {
        std::vector<wil::unique_registry_watcher> result;

        auto addToResult = [&](Manifest::ScopeEnum scopeToUse)
            {
                for (auto architecture : Utility::GetApplicableArchitectures())
                {
                    Registry::Key arpRootKey = GetARPKey(scopeToUse, architecture);

                    if (arpRootKey)
                    {
                        result.emplace_back(wil::make_registry_watcher(arpRootKey, L"", true, [scopeToUse, architecture, callback](wil::RegistryChangeKind change) { callback(scopeToUse, architecture, change); }));
                    }
                }
            };

        if (scope == Manifest::ScopeEnum::Unknown)
        {
            addToResult(Manifest::ScopeEnum::User);
            addToResult(Manifest::ScopeEnum::Machine);
        }
        else
        {
            addToResult(scope);
        }

        return result;
    }
}
