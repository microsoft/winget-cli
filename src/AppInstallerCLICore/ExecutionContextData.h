// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerDownloader.h>
#include <winget/RepositorySource.h>
#include <winget/Manifest.h>
#include <winget/ARPCorrelation.h>
#include <winget/Authentication.h>
#include <winget/Pin.h>
#include <winget/PinningData.h>
#include "CompletionData.h"
#include "PackageCollection.h"
#include "PortableInstaller.h"
#include "Workflows/WorkflowBase.h"
#include "ConfigurationContext.h"

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
        SearchRequest, // Only set for multiple installs
        SearchResult,
        SourceList,
        Package,
        Manifest,
        PackageVersion,
        Installer,
        DownloadHashInfo,
        InstallerPath,
        LogPath,
        InstallerArgs,
        OperationReturnCode,
        CompletionData,
        InstalledPackageVersion,
        UninstallString,
        PackageFamilyNames,
        ProductCodes,
        // On export: A collection of packages to be exported to a file
        // On import: A collection of packages read from a file
        PackageCollection,
        // When installing multiple packages at once (upgrade all, import, install with multiple args, dependencies):
        // A collection of sub-contexts, each of which handles the installation of a single package.
        PackageSubContexts,
        // On import: Sources for the imported packages
        Sources,
        ARPCorrelationData,
        CorrelatedAppsAndFeaturesEntries,
        Dependencies,
        DependencySource,
        AllowedArchitectures,
        AllowUnknownScope,
        PortableInstaller,
        PinningData,
        Pins,
        ConfigurationContext,
        DownloadDirectory,
        ModifyPath,
        RepairString,
        MsixDigests,
        InstallerDownloadAuthenticators,
        Max
    };

    struct Context;

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
            using value_t = Repository::Source;
        };

        template <>
        struct DataMapping<Data::SearchRequest>
        {
            using value_t = Repository::SearchRequest;
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
            using value_t = std::shared_ptr<Repository::ICompositePackage>;
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
        struct DataMapping<Data::DownloadHashInfo>
        {
            using value_t = std::pair<std::vector<uint8_t>, Utility::DownloadResult>;
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
        struct DataMapping<Data::OperationReturnCode>
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
        struct DataMapping<Data::PackageSubContexts>
        {
            using value_t = std::vector<std::unique_ptr<Context>>;
        };

        template <>
        struct DataMapping<Data::Sources>
        {
            using value_t = std::vector<Repository::Source>;
        };

        template <>
        struct DataMapping<Data::ARPCorrelationData>
        {
            using value_t = Repository::Correlation::ARPCorrelationData;
        };

        template <>
        struct DataMapping<Data::CorrelatedAppsAndFeaturesEntries>
        {
            using value_t = std::vector<Manifest::AppsAndFeaturesEntry>;
        };

        template <>
        struct DataMapping<Data::Dependencies>
        {
            using value_t = Manifest::DependencyList;
        };

        template <>
        struct DataMapping<Data::DependencySource>
        {
            using value_t = Repository::Source;
        };
        
        template <>
        struct DataMapping<Data::AllowedArchitectures>
        {
            using value_t = std::vector<Utility::Architecture>;
        };

        template <>
        struct DataMapping<Data::AllowUnknownScope>
        {
            using value_t = bool;
        };

        template <>
        struct DataMapping<Data::PortableInstaller>
        {
            using value_t = CLI::Portable::PortableInstaller;
        };

        template <>
        struct DataMapping<Data::PinningData>
        {
            using value_t = Pinning::PinningData;
        };

        template <>
        struct DataMapping<Data::Pins>
        {
            using value_t = std::vector<Pinning::Pin>;
        };

        template <>
        struct DataMapping<Data::ConfigurationContext>
        {
            using value_t = ConfigurationContext;
        };

        template <>
        struct DataMapping<Data::DownloadDirectory>
        {
            using value_t = std::filesystem::path;
        };

        template<>
        struct DataMapping<Data::ModifyPath>
        {
            using value_t = std::string;
        };

        template<>
        struct DataMapping<Data::RepairString>
        {
            using value_t = std::string;
        };

        template<>
        struct DataMapping<Data::MsixDigests>
        {
            // The pair is { URL, Digest }
            using value_t = std::vector<std::pair<std::string, std::wstring>>;
        };

        template<>
        struct DataMapping<Data::InstallerDownloadAuthenticators>
        {
            // The authenticator map shared with sub contexts
            using value_t = std::shared_ptr<std::map<Authentication::AuthenticationInfo, Authentication::Authenticator>>;
        };
    }
}
