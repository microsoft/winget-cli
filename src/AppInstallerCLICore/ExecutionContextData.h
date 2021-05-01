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
        CompletionData,
        InstalledPackageVersion,
        UninstallString,
        PackageFamilyNames,
        ProductCodes,
        // On export: A collection of packages to be exported to a file
        // On import: A collection of packages read from a file
        PackageCollection,
        // On import: A collection of specific package versions to install
        PackagesToInstall,
        // On import: Sources for the imported packages
        Sources,
        ARPSnapshot,
        Max
    };

    struct PackageToInstall
    {
        std::shared_ptr<Repository::IPackageVersion> PackageVersion;
        PackageCollection::Package PackageRequest;
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
    }
}
