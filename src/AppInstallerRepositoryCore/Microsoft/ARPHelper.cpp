// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ARPHelper.h"

namespace AppInstaller::Repository::Microsoft
{
    Registry::Key ARPHelper::GetARPKey(Manifest::ScopeEnum scope, Utility::Architecture architecture) const
    {
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

    bool ARPHelper::GetBoolValue(const Registry::Key& arpKey, const std::wstring& name)
    {
        auto value = arpKey[name];
        return (value && value->GetType() == Registry::Value::Type::DWord && value->GetValue<Registry::Value::Type::DWord>());
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

    void ARPHelper::AddMetadataIfPresent(const Registry::Key& key, const std::wstring& name, SQLiteIndex& index, SQLiteIndex::IdType manifestId, PackageVersionMetadata metadata)
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
                std::ostringstream strstr;
                strstr << value->GetValue<Registry::Value::Type::DWord>();
                valueString = strstr.str();
            }

            if (!valueString.empty())
            {
                index.SetMetadataByManifestId(manifestId, metadata, valueString);
            }
        }
    }

    void ARPHelper::PopulateIndexFromARP(SQLiteIndex& index, Manifest::ScopeEnum scope) const
    {
        for (auto architecture : Utility::GetApplicableArchitectures())
        {
            Registry::Key arpRootKey = GetARPKey(scope, architecture);

            if (arpRootKey)
            {
                PopulateIndexFromKey(index, arpRootKey, Manifest::ScopeToString(scope), Utility::ToString(architecture));
            }
        }
    }

    void ARPHelper::PopulateIndexFromKey(SQLiteIndex& index, const Registry::Key& key, std::string_view scope, std::string_view architecture) const
    {
        AICLI_LOG(Repo, Info, << "Examining ARP entries for " << scope << " | " << architecture);

        for (const auto& arpEntry : key)
        {
            std::string productCode = arpEntry.Name();

            Manifest::Manifest manifest;
            manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ "ARP" });

            // Use the key name as the Id, as it is supposed to be unique.
            // TODO: We probably want something better here, like constructing the value as
            //       `Publisher.DisplayName`. We would need to ensure that there are no matches
            //       against the rest of the data however (might happen if same package is
            //       installed for multiple architectures/languages).
            manifest.Id = productCode;

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
            manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(displayNameValue);
            if (displayNameValue.empty())
            {
                AICLI_LOG(Repo, Verbose, << "Skipping " << productCode << " because DisplayName is empty");
                continue;
            }

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

            // TODO: If we want to keep the constructed manifest around to allow for `show` type commands
            //       against installed packages, we should use URLInfoAbout/HelpLink for the Homepage.

            // TODO: Determine the best way to handle duplicates; sometimes the same package will be listed under
            //       both x64 and x86 locations for ARP.
            //       For now, we will attempt to insert and catch.
            std::optional<SQLiteIndex::IdType> manifestIdOpt;

            try
            {
                // Use the ProductCode as a unique key for the path
                manifestIdOpt = index.AddManifest(manifest, Utility::ConvertToUTF16(manifest.Installers[0].ProductCode));
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

            // Pick up Language to enable proper selection of language for upgrade.
            // TODO: Determine if InnoSetupLanguage represents the same concept and pick it up if language is not present.
            AddMetadataIfPresent(arpKey, Language, index, manifestId, PackageVersionMetadata::Locale);

            // Pick up WindowsInstaller to determine if this is an MSI install.
            // TODO: Could also determine Inno (and maybe other types) through detecting other keys here.
            auto installedType = Manifest::InstallerTypeEnum::Exe;

            if (GetBoolValue(arpKey, WindowsInstaller))
            {
                installedType = Manifest::InstallerTypeEnum::Msi;
            }

            index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType, Manifest::InstallerTypeToString(installedType));
        }
    }
}
