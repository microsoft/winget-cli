// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include <filesystem>
#include <winget/ManifestCommon.h>
#include <AppInstallerArchitecture.h>
#include <winget/Manifest.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;

namespace AppInstaller::Portable
{
    struct PortableArguments
    {
        HKEY RootKey;
        std::filesystem::path InstallRootPackageDirectory;      // Portable install root directory including the package subdirectory.
        std::string AppPathEntry;
        std::string PackageId;
        Manifest::AppsAndFeaturesEntry AppsAndFeatureEntry;
    };

    std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum& scope, Utility::Architecture& arch, std::string_view& installLocationArg);

    bool WriteToAppPathsRegistry(HKEY root, std::string_view entryName, const std::filesystem::path& exePath, bool enablePath);

    bool WriteToUninstallRegistry(HKEY root, std::string_view packageIdentifier, Manifest::AppsAndFeaturesEntry& entry);

    bool CleanUpRegistryEdits(HKEY root, std::string& appPathEntry, std::string& productCode);

    DWORD CALLBACK CopyPortableExeProgressCallback(
        LARGE_INTEGER TotalFileSize,
        LARGE_INTEGER TotalBytesTransferred,
        [[maybe_unused]] LARGE_INTEGER StreamSize,
        [[maybe_unused]] LARGE_INTEGER StreamBytesTransferred,
        DWORD dwStreamNumber,
        DWORD dwCallbackReason,
        [[maybe_unused]] HANDLE hSourceFile,
        [[maybe_unused]] HANDLE hDestinationFile,
        LPVOID lpData
    );
}
