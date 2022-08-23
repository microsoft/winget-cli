// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerProgress.h"
#include "winget/ManagedFile.h"
#include "winget/Manifest.h"
#include "winget/MsixManifest.h"

#include <AppxPackaging.h>

#include <wrl/client.h>
#include <winrt/Windows.ApplicationModel.h>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Msix
{
    // Function to create an AppxBundle package reader given the input file name.
    // Returns true if success, false if the input stream is of wrong type.
    bool GetBundleReader(
        IStream* inputStream,
        IAppxBundleReader** reader);

    // Function to create an Appx package reader given the input file name.
    // Returns true if success, false if the input stream is of wrong type.
    bool GetPackageReader(
        IStream* inputStream,
        IAppxPackageReader** reader);

    // Function to create an Appx manifest reader given the input file name.
    void GetManifestReader(
        IStream* inputStream,
        IAppxManifestReader** reader);

    // Gets the package full name from the family name.
    // This will be the one registered for the current user, if any.
    std::optional<std::string> GetPackageFullNameFromFamilyName(std::string_view familyName);

    // Gets the package family name from the given full name.
    std::string GetPackageFamilyNameFromFullName(std::string_view fullName);

    // Gets the package location from the given full name.
    std::optional<std::filesystem::path> GetPackageLocationFromFullName(std::string_view fullName);

    // MsixInfo class handles all appx/msix related query.
    struct MsixInfo
    {
        MsixInfo(std::string_view uriStr);

        template<typename T, std::enable_if_t<std::is_same_v<T, std::filesystem::path>, int> = 0>
        MsixInfo(const T& path) : MsixInfo(path.u8string()) {}

        MsixInfo(const MsixInfo&) = default;
        MsixInfo& operator=(const MsixInfo&) = default;

        MsixInfo(MsixInfo&&) = default;
        MsixInfo& operator=(MsixInfo&&) = default;

        inline bool GetIsBundle()
        {
            return m_isBundle;
        }

        // Full content of AppxSignature.p7x
        // If skipP7xFileId is true, returns content of converted .p7s
        std::vector<byte> GetSignature(bool skipP7xFileId = false);

        // Get the signature sha256 hash.
        Utility::SHA256::HashBuffer GetSignatureHash();

        // Gets the package full name.
        std::wstring GetPackageFullNameWide();
        std::string GetPackageFullName();

        // Gets a value indicating whether the referenced info is newer than the given package.
        bool IsNewerThan(const std::filesystem::path& otherPackage);

        bool IsNewerThan(const winrt::Windows::ApplicationModel::PackageVersion& otherVersion);

        // Writes the package file to the given path.
        void WriteToFile(std::string_view packageFile, const std::filesystem::path& target, IProgressCallback& progress);

        // Writes the package's manifest to the given path.
        void WriteManifestToFile(const std::filesystem::path& target, IProgressCallback& progress);

        // Writes the package file to the given file handle.
        void WriteToFileHandle(std::string_view packageFile, HANDLE target, IProgressCallback& progress);

        // Get application package manifests from msix and msixbundle.
        std::vector<MsixPackageManifest> GetAppPackageManifests() const;

    private:
        bool m_isBundle;
        Microsoft::WRL::ComPtr<IStream> m_stream;
        Microsoft::WRL::ComPtr<IAppxBundleReader> m_bundleReader;
        Microsoft::WRL::ComPtr<IAppxPackageReader> m_packageReader;

        // Get application packages.
        std::vector<Microsoft::WRL::ComPtr<IAppxPackageReader>> GetAppPackages() const;
    };

    struct GetCertContextResult
    {
        wil::unique_cert_context CertContext;
        wil::unique_hcertstore CertStore;
    };

    // Get cert context from a signed msix/msixbundle file.
    GetCertContextResult GetCertContextFromMsix(const std::filesystem::path& msixPath);

    struct WriteLockedMsixFile
    {
        WriteLockedMsixFile(const std::filesystem::path& path);

        bool ValidateTrustInfo(bool checkMicrosoftOrigin) const;

    private:
        Utility::ManagedFile m_file;
    };
}