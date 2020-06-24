// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerProgress.h>
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

        MsixInfo(const MsixInfo&) = default;
        MsixInfo& operator=(const MsixInfo&) = default;

        MsixInfo(MsixInfo&&) = default;
        MsixInfo& operator=(MsixInfo&&) = default;

        inline bool GetIsBundle()
        {
            return m_isBundle;
        }

        // Full content of AppxSignature.p7x
        std::vector<byte> GetSignature();

        // Gets the package full name.
        std::string GetPackageFullName();

        // Gets a value indicating whether the referenced info is newer than the given manifest.
        bool IsNewerThan(const std::filesystem::path& otherManifest);

        bool IsNewerThan(const winrt::Windows::ApplicationModel::PackageVersion& otherVersion);

        // Writes the package file to the given path.
        void WriteToFile(std::string_view packageFile, const std::filesystem::path& target, IProgressCallback& progress);

        // Writes the package's manifest to the given path.
        void WriteManifestToFile(const std::filesystem::path& target, IProgressCallback& progress);

    private:
        bool m_isBundle;
        Microsoft::WRL::ComPtr<IStream> m_stream;
        Microsoft::WRL::ComPtr<IAppxBundleReader> m_bundleReader;
        Microsoft::WRL::ComPtr<IAppxPackageReader> m_packageReader;
    };
}