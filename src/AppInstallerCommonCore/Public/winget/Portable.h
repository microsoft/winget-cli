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
        std::filesystem::path InstallRootDirectory;      // Root directory where portables apps are installed to.
        std::filesystem::path LinksLocation;             // Directory containing the symlinks that point to the portable exes.
        std::string CommandAlias;
        std::string FileName;
        std::string PackageId;
        Manifest::AppsAndFeaturesEntry AppsAndFeatureEntry;
    };

    std::filesystem::path GetPortableInstallRoot(Manifest::ScopeEnum& scope, Utility::Architecture& arch);

    std::filesystem::path GetPortableLinksLocation(Manifest::ScopeEnum& scope, Utility::Architecture& arch);

    void CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link);

    bool AddToPathEnvironmentRegistry(HKEY root, const std::string& keyValue);

    bool WriteToUninstallRegistry(HKEY root, std::string_view packageIdentifier, Manifest::AppsAndFeaturesEntry& entry);

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