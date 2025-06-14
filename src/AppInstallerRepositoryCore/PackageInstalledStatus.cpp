// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/InstalledStatus.h"
#include "Public/winget/PackageVersionSelection.h"
#include <winget/Filesystem.h>

using namespace AppInstaller::Settings;
using namespace std::chrono_literals;

namespace AppInstaller::Repository
{
    namespace
    {
        HRESULT CheckInstalledLocationStatus(const std::filesystem::path& installedLocation)
        {
            HRESULT installLocationStatus = WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE;
            if (!installedLocation.empty())
            {
                std::error_code error;
                installLocationStatus =
                    std::filesystem::exists(installedLocation, error) && std::filesystem::is_directory(installedLocation, error) ?
                    WINGET_INSTALLED_STATUS_INSTALL_LOCATION_FOUND :
                    WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_FOUND;
            }

            return installLocationStatus;
        }

        struct FilePathComparator
        {
            bool operator()(const std::filesystem::path& a, const std::filesystem::path& b) const
            {
                if (std::filesystem::equivalent(a, b))
                {
                    return false;
                }

                return a < b;
            }
        };
        using FileHashMap = std::map<std::filesystem::path, Utility::SHA256::HashBuffer, FilePathComparator>;

        HRESULT CheckInstalledFileStatus(
            const std::filesystem::path& filePath,
            const Utility::SHA256::HashBuffer& expectedHash,
            FileHashMap& fileHashes)
        {
            HRESULT fileStatus = WINGET_INSTALLED_STATUS_FILE_NOT_FOUND;
            try
            {
                if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath))
                {
                    fileStatus = WINGET_INSTALLED_STATUS_FILE_FOUND_WITHOUT_HASH_CHECK;
                    if (!expectedHash.empty())
                    {
                        auto itr = fileHashes.find(filePath);
                        if (itr == fileHashes.end())
                        {
                            std::ifstream in{ filePath, std::ifstream::binary };
                            itr = fileHashes.emplace(filePath, Utility::SHA256::ComputeHash(in)).first;
                        }

                        fileStatus = Utility::SHA256::AreEqual(expectedHash, itr->second) ?
                            WINGET_INSTALLED_STATUS_FILE_HASH_MATCH : WINGET_INSTALLED_STATUS_FILE_HASH_MISMATCH;
                    }
                }
            }
            catch (...)
            {
                fileStatus = WINGET_INSTALLED_STATUS_FILE_ACCESS_ERROR;
            }

            return fileStatus;
        }

        std::vector<InstallerInstalledStatus> CheckInstalledStatusInternal(
            const std::shared_ptr<ICompositePackage>& package,
            InstalledStatusType checkTypes)
        {
            using namespace AppInstaller::Manifest;

            std::vector<InstallerInstalledStatus> result;
            bool checkFileHash = false;
            std::shared_ptr<IPackageVersion> availableVersion;
            FileHashMap fileHashes;

            InstallerTypeEnum installedType = InstallerTypeEnum::Unknown;
            ScopeEnum installedScope = ScopeEnum::Unknown;
            std::filesystem::path installedLocation;
            std::string installedLocale;
            Utility::Architecture installedArchitecture = Utility::Architecture::Unknown;
            HRESULT installedLocationStatus = WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE;

            std::shared_ptr<IPackageVersionCollection> availableVersions = GetAvailableVersionsForInstalledVersion(package);
            auto installedVersions = package->GetAvailablePackage()->GetInstalledVersions();

            // Loop through all installed versions to find a match
            for (const auto& installedVersion : installedVersions)
            {
                auto installedMetadata = installedVersion->GetMetadata();
                installedType = ConvertToInstallerTypeEnum(installedMetadata[PackageVersionMetadata::InstalledType]);
                installedScope = ConvertToScopeEnum(installedMetadata[PackageVersionMetadata::InstalledScope]);
                installedLocation = Filesystem::GetExpandedPath(installedMetadata[PackageVersionMetadata::InstalledLocation]);
                installedLocale = installedMetadata[PackageVersionMetadata::InstalledLocale];
                installedArchitecture = Utility::ConvertToArchitectureEnum(installedMetadata[PackageVersionMetadata::InstalledArchitecture]);
                installedLocationStatus = CheckInstalledLocationStatus(installedLocation);

                Utility::Version installedVersionAsVersion{ installedVersion->GetProperty(PackageVersionProperty::Version) };
                auto installedChannel = installedVersion->GetProperty(PackageVersionProperty::Channel);
                PackageVersionKey versionKey;
                versionKey.Channel = installedChannel.get();

                if (installedVersionAsVersion.IsApproximate())
                {
                    versionKey.Version = installedVersionAsVersion.GetBaseVersion().ToString();
                }
                else
                {
                    versionKey.Version = installedVersionAsVersion.ToString();
                }

                availableVersion = availableVersions->GetVersion(versionKey);
                if (availableVersion)
                {
                    checkFileHash = true;
                    break;
                }
            }

            if (!availableVersion)
            {
                availableVersion = availableVersions->GetLatestVersion();
                THROW_HR_IF(E_UNEXPECTED, !availableVersion);
            }

            auto manifest = availableVersion->GetManifest();
            for (auto const& installer : manifest.Installers)
            {
                InstallerInstalledStatus installerStatus;
                installerStatus.Installer = installer;

                if (WI_IsAnyFlagSet(checkTypes, InstalledStatusType::AllAppsAndFeaturesEntryChecks))
                {
                    bool isMatchingInstaller =
                        !installedVersions.empty() &&
                        IsInstallerTypeCompatible(installedType, installer.EffectiveInstallerType()) &&
                        (installedScope == ScopeEnum::Unknown || installer.Scope == ScopeEnum::Unknown || installedScope == installer.Scope) &&
                        (installedArchitecture == Utility::Architecture::Unknown || installer.Arch == Utility::Architecture::Neutral || installedArchitecture == installer.Arch) &&
                        (installedLocale.empty() || installer.Locale.empty() || !Locale::IsWellFormedBcp47Tag(installedLocale) || Locale::GetDistanceOfLanguage(installedLocale, installer.Locale) >= Locale::MinimumDistanceScoreAsCompatibleMatch);

                    if (WI_IsFlagSet(checkTypes, InstalledStatusType::AppsAndFeaturesEntry))
                    {
                        installerStatus.Status.emplace_back(
                            InstalledStatusType::AppsAndFeaturesEntry,
                            "",
                            isMatchingInstaller ? WINGET_INSTALLED_STATUS_ARP_ENTRY_FOUND : WINGET_INSTALLED_STATUS_ARP_ENTRY_NOT_FOUND);
                    }

                    if (isMatchingInstaller && WI_IsFlagSet(checkTypes, InstalledStatusType::AppsAndFeaturesEntryInstallLocation))
                    {
                        installerStatus.Status.emplace_back(
                            InstalledStatusType::AppsAndFeaturesEntryInstallLocation,
                            installedLocation.u8string(),
                            installedLocationStatus);
                    }

                    if (isMatchingInstaller &&
                        installedLocationStatus == WINGET_INSTALLED_STATUS_INSTALL_LOCATION_FOUND &&
                        WI_IsFlagSet(checkTypes, InstalledStatusType::AppsAndFeaturesEntryInstallLocationFile))
                    {
                        for (auto const& file : installer.InstallationMetadata.Files)
                        {
                            std::filesystem::path filePath = installedLocation / Utility::ConvertToUTF16(file.RelativeFilePath);
                            auto fileStatus = CheckInstalledFileStatus(filePath, checkFileHash ? file.FileSha256 : Utility::SHA256::HashBuffer{}, fileHashes);

                            installerStatus.Status.emplace_back(
                                InstalledStatusType::AppsAndFeaturesEntryInstallLocationFile,
                                filePath.u8string(),
                                fileStatus);
                        }
                    }
                }

                if (WI_IsAnyFlagSet(checkTypes, InstalledStatusType::AllDefaultInstallLocationChecks) && installer.InstallationMetadata.HasData())
                {
                    auto defaultInstalledLocation = Filesystem::GetExpandedPath(installer.InstallationMetadata.DefaultInstallLocation);
                    HRESULT defaultInstalledLocationStatus = CheckInstalledLocationStatus(defaultInstalledLocation);

                    if (WI_IsFlagSet(checkTypes, InstalledStatusType::DefaultInstallLocation))
                    {
                        installerStatus.Status.emplace_back(
                            InstalledStatusType::DefaultInstallLocation,
                            defaultInstalledLocation.u8string(),
                            defaultInstalledLocationStatus);
                    }

                    if (defaultInstalledLocationStatus == WINGET_INSTALLED_STATUS_INSTALL_LOCATION_FOUND &&
                        WI_IsFlagSet(checkTypes, InstalledStatusType::DefaultInstallLocationFile))
                    {
                        for (auto const& file : installer.InstallationMetadata.Files)
                        {
                            std::filesystem::path filePath = defaultInstalledLocation / Utility::ConvertToUTF16(file.RelativeFilePath);
                            auto fileStatus = CheckInstalledFileStatus(filePath, checkFileHash ? file.FileSha256 : Utility::SHA256::HashBuffer{}, fileHashes);

                            installerStatus.Status.emplace_back(
                                InstalledStatusType::DefaultInstallLocationFile,
                                filePath.u8string(),
                                fileStatus);
                        }
                    }
                }

                if (!installerStatus.Status.empty())
                {
                    result.emplace_back(std::move(installerStatus));
                }
            }

            return result;
        }
    }

    std::vector<InstallerInstalledStatus> CheckPackageInstalledStatus(const std::shared_ptr<ICompositePackage>& package, InstalledStatusType checkTypes)
    {
        return CheckInstalledStatusInternal(package, checkTypes);
    }
}
