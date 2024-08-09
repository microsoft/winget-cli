// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "AppInstallerRuntime.h"
#include "AppInstallerDownloader.h"
#include "winget/MsixManifestValidation.h"

namespace AppInstaller::Manifest
{
    std::vector<ValidationError> MsixManifestValidation::Validate(
        const Manifest& manifest,
        const ManifestInstaller& installer)
    {
        std::vector<ValidationError> errors;
        auto msixInfo = GetMsixInfo(installer.Url);
        if (msixInfo)
        {
            ValidateMsixManifestSignatureHash(msixInfo, installer.SignatureSha256, errors);
            auto msixManifests = msixInfo->GetAppPackageManifests();
            auto installerMinOSVersion = GetManifestInstallerMinOSVersion(installer.MinOSVersion, errors);
            for (const auto& msixManifest : msixManifests)
            {
                auto msixManifestIdentity = msixManifest.GetIdentity();
                ValidateMsixManifestPackageFamilyName(msixManifestIdentity.GetPackageFamilyName(), installer.PackageFamilyName, errors);
                ValidateMsixManifestPackageVersion(msixManifestIdentity.GetVersion(), manifest.Version, errors);
                ValidateMsixManifestMinOSVersion(msixManifest.GetMinimumOSVersionForSupportedPlatforms(), installerMinOSVersion, installer.Url, errors);
            }
        }
        else
        {
            errors.emplace_back(ManifestError::InstallerFailedToProcess, "InstallerUrl", installer.Url);
        }

        return errors;
    }

    MsixManifestValidation::~MsixManifestValidation()
    {
        AICLI_LOG(Core, Info, << "Removing downloaded installers");
        for (const auto& installerPath : m_downloadedInstallers)
        {
            try
            {
                std::filesystem::remove(installerPath);
            }
            catch (...)
            {
                AICLI_LOG(Core, Warning, << "Failed to remove downloaded installer: " << installerPath);
            }
        }
    }

    std::optional<std::filesystem::path> MsixManifestValidation::DownloadInstaller(std::string installerUrl, int retryCount)
    {
        while (retryCount-- > 0)
        {
            try
            {
                AICLI_LOG(Core, Info, << "Start downloading installer");
                auto tempFile = Runtime::GetNewTempFilePath();
                ProgressCallback emptyCallback;
                Utility::Download(installerUrl, tempFile, Utility::DownloadType::Installer, emptyCallback);
                m_downloadedInstallers.push_back(tempFile);
                return tempFile;
            }
            catch (...)
            {
                AICLI_LOG(Core, Error, << "Downloading installer failed. Remaining attempts: " << retryCount);
            }
        }

        return std::nullopt;
    }

    std::shared_ptr<Msix::MsixInfo> MsixManifestValidation::GetMsixInfoFromUrl(std::string installerUrl)
    {
        try
        {
            AICLI_LOG(Core, Info, << "Fetching Msix info from installer url");
            return std::make_shared<Msix::MsixInfo>(installerUrl);
        }
        catch (...)
        {
            AICLI_LOG(Core, Error, << "Error fetching Msix info from the installer url.");
            return nullptr;
        }
    }

    std::shared_ptr<Msix::MsixInfo> MsixManifestValidation::GetMsixInfoFromLocalPath(std::string installerUrl)
    {
        int maxRetry = 3;
        std::shared_ptr<Msix::MsixInfo> msixInfo;
        auto installerPath = DownloadInstaller(installerUrl, maxRetry);
        if (installerPath.has_value())
        {
            try
            {
                AICLI_LOG(Core, Info, << "Fetching Msix info from installer local path");
                msixInfo = std::make_shared<Msix::MsixInfo>(installerPath.value());
            }
            catch (...)
            {
                AICLI_LOG(Core, Error, << "Error fetching Msix info from the installer local path.");
            }
        }
        else
        {
            AICLI_LOG(Core, Error, << "Failed to download installer.");
        }

        return msixInfo;
    }

    std::shared_ptr<Msix::MsixInfo> MsixManifestValidation::GetMsixInfo(std::string installerUrl)
    {
        std::shared_ptr<Msix::MsixInfo> msixInfo;
        // Cache Msix info for new installer url
        auto findMsixInfo = m_msixInfoCache.find(installerUrl);
        if (findMsixInfo == m_msixInfoCache.end())
        {
            msixInfo = GetMsixInfoFromUrl(installerUrl);
            if (!msixInfo)
            {
                AICLI_LOG(Core, Warning, << "Failed to get Msix info directly from the installer url. "
                    << "Downloading installer instead.");
                msixInfo = GetMsixInfoFromLocalPath(installerUrl);
            }

            if (msixInfo)
            {
                m_msixInfoCache.insert({ installerUrl, msixInfo });
            }
            else
            {
                AICLI_LOG(Core, Error, << "Msix info could not be obtained.");
            }
        }
        else
        {
            msixInfo = findMsixInfo->second;
        }

        return msixInfo;
    }

    std::optional<Msix::OSVersion> MsixManifestValidation::GetManifestInstallerMinOSVersion(
        std::string minOSVersion,
        std::vector<ValidationError>& errors)
    {
        try
        {
            if (!minOSVersion.empty())
            {
                return std::make_optional<Msix::OSVersion>(minOSVersion);
            }
        }
        catch (const std::exception&)
        {
            errors.emplace_back(ManifestError::InvalidFieldValue, "MinimumOSVersion", minOSVersion);
        }
        return std::nullopt;
    }

    void MsixManifestValidation::ValidateMsixManifestPackageFamilyName(
        Utility::NormalizedString msixPackageFamilyName,
        Utility::NormalizedString manifestPackageFamilyName,
        std::vector<ValidationError>& errors)
    {
        if (!manifestPackageFamilyName.empty())
        {
            if (manifestPackageFamilyName != msixPackageFamilyName)
            {
                errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", msixPackageFamilyName);
            }
        }
        else
        {
            errors.emplace_back(ManifestError::OptionalFieldMissing, "PackageFamilyName", msixPackageFamilyName, m_validationErrorLevel);
        }
    }

    void MsixManifestValidation::ValidateMsixManifestPackageVersion(
        const Msix::PackageVersion& msixPackageVersion,
        const string_t& manifestPackageVersionStr,
        std::vector<ValidationError>& errors)
    {
        std::optional<Msix::PackageVersion> manifestPackageVersion;
        try
        {
            manifestPackageVersion = { manifestPackageVersionStr };
        }
        catch (...)
        {
            AICLI_LOG(Core, Error, << "Failed to parse package version to UINT64");
        }

        if (!manifestPackageVersion.has_value() || msixPackageVersion != manifestPackageVersion.value())
        {
            errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "PackageVersion", msixPackageVersion.ToString());
        }
    }

    void MsixManifestValidation::ValidateMsixManifestMinOSVersion(
        const std::optional<Msix::OSVersion>& msixMinOSVersion,
        const std::optional<Msix::OSVersion>& manifestMinOSVersion,
        std::string installerUrl,
        std::vector<ValidationError>& errors)
    {
        if (!msixMinOSVersion.has_value())
        {
            errors.emplace_back(ManifestError::NoSupportedPlatforms, "InstallerUrl", installerUrl);
        }
        else if (manifestMinOSVersion.has_value())
        {
            if (msixMinOSVersion.value() != manifestMinOSVersion.value())
            {
                errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", msixMinOSVersion.value().ToString());
            }
        }
        else
        {
            errors.emplace_back(
                ManifestError::OptionalFieldMissing,
                "MinimumOSVersion",
                msixMinOSVersion.value().ToString(),
                m_validationErrorLevel);
        }
    }

    void MsixManifestValidation::ValidateMsixManifestSignatureHash(
        const std::shared_ptr<Msix::MsixInfo> msixInfo,
        const Utility::SHA256::HashBuffer& manifestSignatureHash,
        std::vector<ValidationError>& errors)
    {
        try
        {
            if (!manifestSignatureHash.empty())
            {
                auto msixSignatureHash = msixInfo->GetSignatureHash();
                if (msixSignatureHash != manifestSignatureHash)
                {
                    auto msixSignatureHashString = Utility::SHA256::ConvertToString(msixSignatureHash);
                    errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "SignatureSha256", msixSignatureHashString);
                }
            }
        }
        catch (const wil::ResultException&)
        {
            errors.emplace_back(ManifestError::MsixSignatureHashFailed);
        }
    }
}
