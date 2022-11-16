// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerErrors.h"
#include "AppInstallerMsixInfo.h"
#include "winget/Manifest.h"
#include "winget/ManifestValidation.h"

namespace AppInstaller::Manifest
{
    struct MsixManifestValidation
    {
        MsixManifestValidation(ValidationError::Level validationErrorLevel) : m_validationErrorLevel(validationErrorLevel) {}

        ~MsixManifestValidation();

        // Validate manifest for Msix packages and Msix bundles.
        std::vector<ValidationError> Validate(
            const Manifest& manifest,
            const ManifestInstaller& installer);
    private:
        std::map<std::string, std::shared_ptr<Msix::MsixInfo>> m_msixInfoCache;
        std::vector<std::filesystem::path> m_downloadedInstallers;
        ValidationError::Level m_validationErrorLevel;

        // Get Msix info from url/local path, or load it from cache.
        // Return null pointer if operation failed.
        std::shared_ptr<Msix::MsixInfo> GetMsixInfo(std::string installerUrl);

        // Get Msix info from installer url.
        // Return null pointer if operation failed.
        std::shared_ptr<Msix::MsixInfo> GetMsixInfoFromUrl(std::string installerUrl);

        // Download and get msix info from installer local path.
        // Return null pointer if operation failed.
        std::shared_ptr<Msix::MsixInfo> GetMsixInfoFromLocalPath(std::string installerUrl);

        // Download the installer.
        // If the download was successful, return the destination path.
        std::optional<std::filesystem::path> DownloadInstaller(std::string installerUrl, int retryCount);

        // Get manifest installer minimum OS version or nullopt if failed to
        // parse input.
        std::optional<Msix::OSVersion> GetManifestInstallerMinOSVersion(
            std::string minOSVersion,
            std::vector<ValidationError>& errors);

        // Validate Msix package family name.
        void ValidateMsixManifestPackageFamilyName(
            Utility::NormalizedString msixPackageFamilyName,
            Utility::NormalizedString manifestPackageFamilyName,
            std::vector<ValidationError>& errors);

        // Validate Msix package version.
        void ValidateMsixManifestPackageVersion(
            const Msix::PackageVersion& msixPackageVersion,
            const string_t& manifestPackageVersionStr,
            std::vector<ValidationError>& errors);

        // Validate Msix minimum OS version for supported platforms.
        void ValidateMsixManifestMinOSVersion(
            const std::optional<Msix::OSVersion>& msixMinOSVersion,
            const std::optional<Msix::OSVersion>& manifestMinOSVersion,
            std::string installerUrl,
            std::vector<ValidationError>& errors);

        // Validate Msix signature hash.
        void ValidateMsixManifestSignatureHash(
            const std::shared_ptr<Msix::MsixInfo> msixInfo,
            const Utility::SHA256::HashBuffer& manifestSignatureHash,
            std::vector<ValidationError>& errors);
    };
}
