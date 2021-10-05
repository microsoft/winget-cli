// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerRepositorySearch.h>
#include <AppInstallerRepositorySource.h>
#include <winget/Manifest.h>
#include "CompletionData.h"
#include "PackageCollection.h"
#include "Workflows/WorkflowBase.h"

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>


namespace AppInstaller::CLI::Execution
{
    // Names a piece of data stored in the context by a workflow step.
    // Must start at 0 to enable direct access to variant in Context.
    // Max must be last and unused.
    enum class Data : size_t
    {
        Source,
        SearchResult,
        SourceList,
        Package,
        Manifest,
        PackageVersion,
        Installer,
        HashPair,
        InstallerPath,
        LogPath,
        InstallerArgs,
        InstallerReturnCode,
        CompletionData,
        InstalledPackageVersion,
        UninstallString,
        PackageFamilyNames,
        ProductCodes,
        // On export: A collection of packages to be exported to a file
        // On import: A collection of packages read from a file
        PackageCollection,
        // On import and upgrade all: A collection of specific package versions to install
        PackagesToInstall,
        // On import: Sources for the imported packages
        Sources,
        ARPSnapshot,
        Dependencies,
        DependencySource,
        AllowedArchitectures,
        Max
    };

    // Contains all the information needed to install a package.
    // This is used when installing multiple packages to pass all the
    // data to a sub-context.
    struct PackageToInstall
    {
        PackageToInstall(
            std::shared_ptr<Repository::IPackageVersion>&& packageVersion,
            std::shared_ptr<Repository::IPackageVersion>&& installedPackageVersion,
            Manifest::Manifest&& manifest,
            Manifest::ManifestInstaller&& installer,
            Manifest::ScopeEnum scope = Manifest::ScopeEnum::Unknown,
            uint32_t packageSubExecutionId = 0)
            : PackageVersion(std::move(packageVersion)), InstalledPackageVersion(std::move(installedPackageVersion)), Manifest(std::move(manifest)), Installer(std::move(installer)), Scope(scope), PackageSubExecutionId(packageSubExecutionId) { }

        std::shared_ptr<Repository::IPackageVersion> PackageVersion;

        // Used to uninstall the old version if needed.
        std::shared_ptr<Repository::IPackageVersion> InstalledPackageVersion;

        // Use this instead of the PackageVersion->GetManifest() as the locale was
        // applied when selecting the installer.
        Manifest::Manifest Manifest;

        Manifest::ManifestInstaller Installer;
        Manifest::ScopeEnum Scope = Manifest::ScopeEnum::Unknown;

        // Use this sub execution id when installing this package so that 
        // install telemetry is captured with the same sub execution id as other events in Search phase.
        uint32_t PackageSubExecutionId = 0;
    };

    namespace details
    {
        template <Data D>
        struct DataMapping
        {
            // value_t type specifies the type of this data
        };

        template <>
        struct DataMapping<Data::Source>
        {
            using value_t = std::shared_ptr<Repository::ISource>;
        };

        template <>
        struct DataMapping<Data::SearchResult>
        {
            using value_t = Repository::SearchResult;
        };

        template <>
        struct DataMapping<Data::SourceList>
        {
            using value_t = std::vector<Repository::SourceDetails>;
        };

        template <>
        struct DataMapping<Data::Package>
        {
            using value_t = std::shared_ptr<Repository::IPackage>;
        };

        template <>
        struct DataMapping<Data::Manifest>
        {
            using value_t = Manifest::Manifest;
        };

        template <>
        struct DataMapping<Data::PackageVersion>
        {
            using value_t = std::shared_ptr<Repository::IPackageVersion>;
        };

        template <>
        struct DataMapping<Data::Installer>
        {
            using value_t = std::optional<Manifest::ManifestInstaller>;
        };

        template <>
        struct DataMapping<Data::HashPair>
        {
            using value_t = std::pair<std::vector<uint8_t>, std::vector<uint8_t>>;
        };

        template <>
        struct DataMapping<Data::InstallerPath>
        {
            using value_t = std::filesystem::path;
        };

        template <>
        struct DataMapping<Data::LogPath>
        {
            using value_t = std::filesystem::path;
        };

        template <>
        struct DataMapping<Data::InstallerArgs>
        {
            using value_t = std::string;
        };

        template <>
        struct DataMapping<Data::InstallerReturnCode>
        {
            using value_t = DWORD;
        };

        template <>
        struct DataMapping<Data::CompletionData>
        {
            using value_t = CLI::CompletionData;
        };

        template <>
        struct DataMapping<Data::InstalledPackageVersion>
        {
            using value_t = std::shared_ptr<Repository::IPackageVersion>;
        };

        template <>
        struct DataMapping<Data::UninstallString>
        {
            using value_t = std::string;
        };

        template <>
        struct DataMapping<Data::PackageFamilyNames>
        {
            using value_t = std::vector<Utility::LocIndString>;
        };

        template <>
        struct DataMapping<Data::ProductCodes>
        {
            using value_t = std::vector<Utility::LocIndString>;
        };

        template <>
        struct DataMapping<Data::PackageCollection>
        {
            using value_t = CLI::PackageCollection;
        };

        template <>
        struct DataMapping<Data::PackagesToInstall>
        {
            using value_t = std::vector<PackageToInstall>;
        };

        template <>
        struct DataMapping<Data::Sources>
        {
            using value_t = std::vector<std::shared_ptr<Repository::ISource>>;
        };

        template <>
        struct DataMapping<Data::ARPSnapshot>
        {
            // Contains the { Id, Version, Channel }
            using value_t = std::vector<std::tuple<Utility::LocIndString, Utility::LocIndString, Utility::LocIndString>>;
        };

        template <>
        struct DataMapping<Data::Dependencies>
        {
            using value_t = Manifest::DependencyList;
        };

        template <>
        struct DataMapping<Data::DependencySource>
        {
            using value_t = std::shared_ptr<Repository::ISource>;
        };
        
        template <>
        struct DataMapping<Data::AllowedArchitectures>
        {
            using value_t = std::vector<Utility::Architecture>;
        };
    }
}
