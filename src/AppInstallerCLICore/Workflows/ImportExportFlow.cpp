// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "ImportExportFlow.h"
#include "PackageCollection.h"
#include "WorkflowBase.h"
#include "CompositeSource.h"
#include "AppInstallerRepositorySearch.h"

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // Gets the available PackageVersion that has the same version as the installed version.
        // The package must have an installed version.
        std::shared_ptr<IPackageVersion> GetAvailableVersionMatchingInstalledVersion(const IPackage& package)
        {
            auto installedVersion = package.GetInstalledVersion();
            PackageVersionKey installedVersionKey
            {
                "",
                installedVersion->GetProperty(PackageVersionProperty::Version).get(),
                installedVersion->GetProperty(PackageVersionProperty::Channel).get(),
            };
            return package.GetAvailableVersion(installedVersionKey);
        }

        // Selects which version of an installed package to list when exporting.
        std::shared_ptr<IPackageVersion> SelectPackageVersionToExport(const IPackage& package)
        {
            // See if the installed version is available from some source
            auto availableVersion = GetAvailableVersionMatchingInstalledVersion(package);
            if (!availableVersion)
            {
                // If the exact version isn't available, list the latest.
                availableVersion = package.GetLatestAvailableVersion();
            }

            if (availableVersion)
            {
                AICLI_LOG(
                    CLI,
                    Info,
                    << "Found package " << availableVersion->GetProperty(PackageVersionProperty::Id)
                    << " " << availableVersion->GetProperty(PackageVersionProperty::Version)
                    << " available from " << availableVersion->GetSource()->GetIdentifier());
                return availableVersion;
            }

            // If there is no available version, we didn't have a mapping for the ARP
            // entry of the package. List the installed version so it can be later installed
            // if we get a better mapping.
            auto installedVersion = package.GetInstalledVersion();
            AICLI_LOG(CLI, Warning, << "No available version of package " << installedVersion->GetProperty(PackageVersionProperty::Id) << " was found to export");
            return installedVersion;
        }
    }

    void SelectVersionsToExport(Execution::Context& context)
    {
        const auto& searchResult = context.Get<Execution::Data::SearchResult>();
        PackageCollection versionsToExport = {};
        auto& requestsFromSource = versionsToExport.RequestsFromSources;
        for (const auto& packageMatch : searchResult.Matches)
        {
            auto packageVersion = SelectPackageVersionToExport(*packageMatch.Package);

            const auto& sourceIdentifier = packageVersion->GetSource()->GetIdentifier();
            auto sourceItr = std::find_if(requestsFromSource.begin(), requestsFromSource.end(), [&](const PackageRequestsFromSource& s) { return s.SourceIdentifier == sourceIdentifier; });
            if (sourceItr == requestsFromSource.end())
            {
                requestsFromSource.emplace_back(Utility::LocIndString{ sourceIdentifier }, packageVersion->GetSource()->GetDetails());
                sourceItr = std::prev(requestsFromSource.end());
            }

            sourceItr->Packages.emplace_back(
                packageVersion->GetProperty(PackageVersionProperty::Id),
                packageVersion->GetProperty(PackageVersionProperty::Version).get(),
                packageVersion->GetProperty(PackageVersionProperty::Channel).get());
        }

        context.Add<Execution::Data::PackageCollection>(std::move(versionsToExport));
    }

    void WriteImportFile(Execution::Context& context)
    {
        auto packages = PackagesJson::CreateJson(context.Get<Execution::Data::PackageCollection>());

        std::filesystem::path outputFilePath{ context.Args.GetArg(Execution::Args::Type::OutputFile) };
        std::ofstream outputFileStream{ outputFilePath };
        outputFileStream << packages;
    }

    void ReadImportFile(Execution::Context& context)
    {
        std::filesystem::path importFilePath{ context.Args.GetArg(Execution::Args::Type::ImportFile) };

        // TODO: Handle errors
        Json::Value jsonRoot;
        std::ifstream{ importFilePath } >> jsonRoot;

        auto packages = PackagesJson::ParseJson(jsonRoot);
        context.Add<Execution::Data::PackageCollection>(packages);

        if (packages.RequestsFromSources.empty())
        {
            AICLI_LOG(CLI, Warning, << "No packages to install");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
        }
    }

    void SearchPackagesForImport(Execution::Context& context)
    {
        auto availableSources = Repository::GetSources();
        std::vector<std::shared_ptr<IPackageVersion>> packagesToInstall;

        // Aggregated source with all the required sources.
        // We keep it as the source of the root context to keep all the
        // source objects alive for install.
        auto aggregatedSource = std::make_shared<Repository::CompositeSource>("*ImportSource");

        // Look for the packages needed from each source independently.
        // If a package is available from multiple sources, this ensures we will get it from the right one.
        for (auto& requiredSource : context.Get<Execution::Data::PackageCollection>().RequestsFromSources)
        {
            if (!requiredSource.Details.Name.empty())
            {
                // For packages that come from a specific source, find the matching available source.
                std::optional<SourceDetails> matchingSource = {};
                for (auto& availableSource : availableSources)
                {
                    if (availableSource.Identifier == requiredSource.Details.Identifier)
                    {
                        matchingSource = availableSource;
                        break;
                    }
                }

                if (matchingSource.has_value())
                {
                    requiredSource.Details.Name = matchingSource.value().Name;
                }
                else
                {
                    // TODO: Add option for ignoring/installing missing sources?
                    AICLI_LOG(CLI, Error, << "Missing required source: " << requiredSource.Details.Name);
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
                }
            }

            context << OpenNamedSource(requiredSource.Details.Name);
            auto source = context.Get<Execution::Data::Source>();
            aggregatedSource->AddAvailableSource(source);
            for (const auto& packageRequest : requiredSource.Packages)
            {
                // Search for all the packages in the source
                Logging::SubExecutionTelemetryScope subExecution;

                // We want to do best effort to install all packages regardless of previous failures
                auto searchContextPtr = context.Clone();
                Execution::Context& searchContext = *searchContextPtr;
                searchContext.Add<Execution::Data::Source>(source);

                // TODO: Case insensitive?
                MatchType matchType = MatchType::Exact;

                SearchRequest searchRequest;
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, matchType, packageRequest.Id));

                searchContext.Add<Execution::Data::SearchResult>(source->Search(searchRequest));
                searchContext << EnsureOneMatchFromSearchResult(false);

                if (searchContext.IsTerminated())
                {
                    AICLI_LOG(CLI, Warning, << "Package not found [" << packageRequest.Id << "] in source [" << source->GetIdentifier() << "]");
                    continue;
                }

                if (context.Args.Contains(Execution::Args::Type::ExactVersions))
                {
                    PackageVersionKey requestedVersion
                    {
                        requiredSource.SourceIdentifier.get(),
                        packageRequest.VersionAndChannel.GetVersion().ToString(),
                        packageRequest.VersionAndChannel.GetChannel().ToString(),
                    };

                    // TODO: handle unavailable version
                    packagesToInstall.push_back(searchContext.Get<Execution::Data::Package>()->GetAvailableVersion(requestedVersion));
                }
                else
                {
                    packagesToInstall.push_back(searchContext.Get<Execution::Data::Package>()->GetLatestAvailableVersion());
                }
            }
        }

        context.Add<Execution::Data::PackagesToInstall>(std::move(packagesToInstall));
        context.Add<Execution::Data::Source>(std::move(aggregatedSource));
    }
}