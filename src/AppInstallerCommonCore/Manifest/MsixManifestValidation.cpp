// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "winget/MsixManifestValidation.h"

namespace AppInstaller::Manifest
{
    std::vector<ValidationError> MsixManifestValidation::Validate(
        const Manifest &manifest,
        const ManifestInstaller &installer)
    {
        std::vector<ValidationError> errors;
        Msix::PackageVersion packageVersion(manifest.Version);
        auto msixInfo = GetMsixInfo(installer.Url, errors);
        if (msixInfo)
        {
            ValidateMsixManifestSignatureHash(msixInfo, installer.SignatureSha256, errors);
            auto msixManifests = msixInfo->GetAppPackageManifests();
            auto installerMinOSVersion = GetManifestInstallerMinOSVersion(installer.MinOSVersion, errors);
            for (const auto& msixManifest : msixManifests)
            {
                auto msixManifestIdentity = msixManifest.GetIdentity();
                ValidateMsixManifestPackageFamilyName(msixManifestIdentity.GetPackageFamilyName(), installer.PackageFamilyName, errors);
                ValidateMsixManifestPackageVersion(msixManifestIdentity.GetVersion(), packageVersion, errors);
                ValidateMsixManifestMinOSVersion(msixManifest.GetMinimumOSVersionForSupportedPlatforms(), installerMinOSVersion, installer.Url, errors);
            }
        }

        return errors;
    }

    std::shared_ptr<Msix::MsixInfo> MsixManifestValidation::GetMsixInfo(
        std::string installerUrl,
        std::vector<ValidationError> &errors)
    {
        std::shared_ptr<Msix::MsixInfo> msixInfo;
        try
        {
            // Cache Msix info for new installer url
            auto findMsixInfo = m_msixInfoCache.find(installerUrl);
            if (findMsixInfo == m_msixInfoCache.end())
            {
                msixInfo = std::make_shared<Msix::MsixInfo>(installerUrl);
                m_msixInfoCache.insert({ installerUrl, msixInfo });
            }
            else
            {
                msixInfo = findMsixInfo->second;
            }

            return msixInfo;
        }
        catch (...)
        {
            errors.emplace_back(ManifestError::InstallerFailedToProcess, "InstallerUrl", installerUrl);
        }

        return nullptr;
    }

    std::optional<Msix::OSVersion> MsixManifestValidation::GetManifestInstallerMinOSVersion(
        std::string minOSVersion,
        std::vector<ValidationError> &errors)
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
        std::vector<ValidationError> &errors)
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
        const Msix::PackageVersion &msixPackageVersion,
        const Msix::PackageVersion &manifestPackageVersion,
        std::vector<ValidationError> &errors)
    {
        if (msixPackageVersion != manifestPackageVersion)
        {
            errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "PackageVersion", msixPackageVersion.ToString());
        }
    }

    void MsixManifestValidation::ValidateMsixManifestMinOSVersion(
        const std::optional<Msix::OSVersion>& msixMinOSVersion,
        const std::optional<Msix::OSVersion>& manifestMinOSVersion,
        std::string installerUrl,
        std::vector<ValidationError> &errors)
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
