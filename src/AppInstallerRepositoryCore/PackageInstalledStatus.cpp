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
                // Use the none throw version, if the directory cannot be reached, it's treated as not found and later file checks are not performed.
                std::error_code error;
                installLocationStatus =
                    std::filesystem::exists(installedLocation, error) && std::filesystem::is_directory(installedLocation, error) ?
                    WINGET_INSTALLED_STATUS_INSTALL_LOCATION_FOUND :
                    WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_FOUND;
            }

            return installLocationStatus;
        }

        // Map to cache already calculated file hashes.
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
                            // If not found in cache, compute the hash.
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
            std::shared_ptr<IPackageVersion> installedVersion = GetInstalledVersion(package);
            std::shared_ptr<IPackageVersion> availableVersion;
            FileHashMap fileHashes;

            // Variables for metadata from installed version.
            InstallerTypeEnum installedType = InstallerTypeEnum::Unknown;
            ScopeEnum installedScope = ScopeEnum::Unknown;
            std::filesystem::path installedLocation;
            std::string installedLocale;
            Utility::Architecture installedArchitecture = Utility::Architecture::Unknown;
            HRESULT installedLocationStatus = WINGET_INSTALLED_STATUS_INSTALL_LOCATION_NOT_APPLICABLE;

            std::shared_ptr<IPackageVersionCollection> availableVersions = GetAvailableVersionsForInstalledVersion(package);

            // Prepare installed metadata from installed version.
            // Determine the available package version to be used for installed status checking.
            // Only perform file hash check if we find an available version that matches installed version.
            if (installedVersion)
            {
                // Installed metadata.
                auto installedMetadata = installedVersion->GetMetadata();
                installedType = ConvertToInstallerTypeEnum(installedMetadata[PackageVersionMetadata::InstalledType]);
                installedScope = ConvertToScopeEnum(installedMetadata[PackageVersionMetadata::InstalledScope]);
                installedLocation = Filesystem::GetExpandedPath(installedMetadata[PackageVersionMetadata::InstalledLocation]);
                installedLocale = installedMetadata[PackageVersionMetadata::InstalledLocale];
                installedArchitecture = Utility::ConvertToArchitectureEnum(installedMetadata[PackageVersionMetadata::InstalledArchitecture]);
                installedLocationStatus = CheckInstalledLocationStatus(installedLocation);

                // Determine available version.
                Utility::Version installedVersionAsVersion{ installedVersion->GetProperty(PackageVersionProperty::Version) };
                auto installedChannel = installedVersion->GetProperty(PackageVersionProperty::Channel);
                PackageVersionKey versionKey;
                versionKey.Channel = installedChannel.get();

                if (installedVersionAsVersion.IsApproximate())
                {
                    // Use the base version as available version if installed version is mapped to be an approximate.
                    versionKey.Version = installedVersionAsVersion.GetBaseVersion().ToString();
                    availableVersion = availableVersions->GetVersion(versionKey);
                    // It's unexpected if the installed version is already mapped to some version.
                    THROW_HR_IF(E_UNEXPECTED, !availableVersion);
                }
                else
                {
                    versionKey.Version = installedVersionAsVersion.ToString();
                    availableVersion = availableVersions->GetVersion(versionKey);
                    if (availableVersion)
                    {
                        checkFileHash = true;
                    }
                }
            }

            if (!availableVersion)
            {
                // No installed version, or installed version not found in available versions,
                // then attempt to check installed status using latest version.
                availableVersion = availableVersions->GetLatestVersion();
                THROW_HR_IF(E_UNEXPECTED, !availableVersion);
            }

            auto manifest = availableVersion->GetManifest();
            for (auto const& installer : manifest.Installers)
            {
                InstallerInstalledStatus installerStatus;
                installerStatus.Installer = installer;

                // ARP related checks
                if (WI_IsAnyFlagSet(checkTypes, InstalledStatusType::AllAppsAndFeaturesEntryChecks))
                {
                    bool isMatchingInstaller =
                        installedVersion &&
                        IsInstallerTypeCompatible(installedType, installer.EffectiveInstallerType()) &&
                        (installedScope == ScopeEnum::Unknown || installer.Scope == ScopeEnum::Unknown || installedScope == installer.Scope) &&  // Treat unknown scope as compatible
                        (installedArchitecture == Utility::Architecture::Unknown || installer.Arch == Utility::Architecture::Neutral || installedArchitecture == installer.Arch) &&  // Treat unknown installed architecture as compatible
                        (installedLocale.empty() || installer.Locale.empty() || !Locale::IsWellFormedBcp47Tag(installedLocale) || Locale::GetDistanceOfLanguage(installedLocale, installer.Locale) >= Locale::MinimumDistanceScoreAsCompatibleMatch);  // Treat invalid locale as compatible

                    // ARP entry status
                    if (WI_IsFlagSet(checkTypes, InstalledStatusType::AppsAndFeaturesEntry))
                    {
                        installerStatus.Status.emplace_back(
                            InstalledStatusType::AppsAndFeaturesEntry,
                            "",
                            isMatchingInstaller ? WINGET_INSTALLED_STATUS_ARP_ENTRY_FOUND : WINGET_INSTALLED_STATUS_ARP_ENTRY_NOT_FOUND);
                    }

                    // ARP install location status
                    if (isMatchingInstaller && WI_IsFlagSet(checkTypes, InstalledStatusType::AppsAndFeaturesEntryInstallLocation))
                    {
                        installerStatus.Status.emplace_back(
                            InstalledStatusType::AppsAndFeaturesEntryInstallLocation,
                            installedLocation.u8string(),
                            installedLocationStatus);
                    }

                    // ARP install location files
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

                // Default install location related checks
                if (WI_IsAnyFlagSet(checkTypes, InstalledStatusType::AllDefaultInstallLocationChecks) && installer.InstallationMetadata.HasData())
                {
                    auto defaultInstalledLocation = Filesystem::GetExpandedPath(installer.InstallationMetadata.DefaultInstallLocation);
                    HRESULT defaultInstalledLocationStatus = CheckInstalledLocationStatus(defaultInstalledLocation);

                    // Default install location status
                    if (WI_IsFlagSet(checkTypes, InstalledStatusType::DefaultInstallLocation))
                    {
                        installerStatus.Status.emplace_back(
                            InstalledStatusType::DefaultInstallLocation,
                            defaultInstalledLocation.u8string(),
                            defaultInstalledLocationStatus);
                    }

                    // Default install location files
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
