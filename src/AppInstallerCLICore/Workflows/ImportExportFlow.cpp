// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "ImportExportFlow.h"
#include "PackageCollection.h"
#include "WorkflowBase.h"
#include "AppInstallerRepositorySearch.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Repository;

    void SelectVersionsToExport(Execution::Context& context)
    {
        const auto& searchResult = context.Get<Execution::Data::SearchResult>();
        PackageCollection exportedPackages = {};
        auto& exportedSources = exportedPackages.Sources;
        for (const auto& packageMatch : searchResult.Matches)
        {
            auto installedPackageVersion = packageMatch.Package->GetInstalledVersion();
            auto version = installedPackageVersion->GetProperty(PackageVersionProperty::Version);
            auto channel = installedPackageVersion->GetProperty(PackageVersionProperty::Channel);

            // Find an available version of this package to determine its source.
            auto availablePackageVersion = packageMatch.Package->GetAvailableVersion({ "", version.get(), channel.get() });
            if (!availablePackageVersion)
            {
                availablePackageVersion = packageMatch.Package->GetLatestAvailableVersion();
                if (!availablePackageVersion)
                {
                    // Report package not found and move to next package.
                    AICLI_LOG(CLI, Warning, << "No available version of package [" << installedPackageVersion->GetProperty(PackageVersionProperty::Name) << "] was found to export");
                    context.Reporter.Warn() << Resource::String::InstalledPackageNotAvailable << ' ' << installedPackageVersion->GetProperty(PackageVersionProperty::Name) << std::endl;
                    continue;
                }
                else
                {
                    // Warn installed version is not available.
                    AICLI_LOG(
                        CLI,
                        Info,
                        << "Installed package version not available. Package Id [" << availablePackageVersion->GetProperty(PackageVersionProperty::Id) << "]"
                        << ", Version [" << version << "], Channel [" << channel << "]");
                    context.Reporter.Warn() << Resource::String::InstalledPackageVersionNotAvailable << ' ' << availablePackageVersion->GetProperty(PackageVersionProperty::Id) << std::endl;
                }
            }

            const auto& sourceIdentifier = availablePackageVersion->GetSource()->GetIdentifier();
            AICLI_LOG(CLI, Info,
                << "Installed package is available. Package Id [" << availablePackageVersion->GetProperty(PackageVersionProperty::Id) << "], Source [" << sourceIdentifier << "]");

            // Find the exported source for this package
            auto sourceItr = std::find_if(exportedSources.begin(), exportedSources.end(), [&](const PackageCollection::Source& s) { return s.Details.Identifier == sourceIdentifier; });
            if (sourceItr == exportedSources.end())
            {
                exportedSources.emplace_back(availablePackageVersion->GetSource()->GetDetails());
                sourceItr = std::prev(exportedSources.end());
            }

            // Take the Id from the available package because that is the one used in the source,
            // but take the exported version from the installed package.
            sourceItr->Packages.emplace_back(
                availablePackageVersion->GetProperty(PackageVersionProperty::Id),
                version.get(),
                channel.get());
        }

        context.Add<Execution::Data::PackageCollection>(std::move(exportedPackages));
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
        if (packages.Sources.empty())
        {
            AICLI_LOG(CLI, Warning, << "No packages to install");
            context.Reporter.Info() << Resource::String::NoPackagesFoundInImportFile << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
        }

        context.Add<Execution::Data::PackageCollection>(packages);
    }

    void SearchPackagesForImport(Execution::Context& context)
    {
        auto availableSources = Repository::GetSources();
        std::vector<std::shared_ptr<IPackageVersion>> packagesToInstall = {};
        bool foundAll = true;

        // List of all the sources used for import.
        // Needed to keep all the source objects alive for install.
        std::vector<std::shared_ptr<ISource>> sources = {};

        // Look for the packages needed from each source independently.
        // If a package is available from multiple sources, this ensures we will get it from the right one.
        for (auto& requiredSource : context.Get<Execution::Data::PackageCollection>().Sources)
        {
            // Find the installed source matching the one described in the collection.
            auto matchingSource = std::find_if(
                availableSources.begin(),
                availableSources.end(),
                [&](const SourceDetails& s) { return s.Identifier == requiredSource.Details.Identifier; });
            if (matchingSource != availableSources.end())
            {
                requiredSource.Details.Name = matchingSource->Name;
            }
            else
            {
                AICLI_LOG(CLI, Error, << "Missing required source: " << requiredSource.Details.Name);
                context.Reporter.Warn() << Resource::String::ImportSourceNotInstalled << ' ' << requiredSource.Details.Name << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            }

            // Search for all the packages in the source.
            // Each search is done in a sub context to search everything regardless of previous failures.
            context << OpenNamedSource(requiredSource.Details.Name);
            auto source = context.Get<Execution::Data::Source>();
            for (const auto& packageRequest : requiredSource.Packages)
            {
                Logging::SubExecutionTelemetryScope subExecution;

                auto searchContextPtr = context.Clone();
                Execution::Context& searchContext = *searchContextPtr;
                searchContext.Add<Execution::Data::Source>(source);

                // Search for the current package
                SearchRequest searchRequest;
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, packageRequest.Id));
                searchContext.Add<Execution::Data::SearchResult>(source->Search(searchRequest));

                Utility::VersionAndChannel versionAndChannel = {};
                if (context.Args.Contains(Execution::Args::Type::ExactVersions))
                {
                    versionAndChannel = packageRequest.VersionAndChannel;
                }

                // Find the single version we want is available
                searchContext <<
                    Workflow::EnsureOneMatchFromSearchResult(false) <<
                    Workflow::GetManifestWithVersionFromPackage(versionAndChannel);

                if (searchContext.IsTerminated())
                {
                    AICLI_LOG(CLI, Info, << "Package not found for import: [" << packageRequest.Id << "], Version " << versionAndChannel.ToString());
                    context.Reporter.Info() << Resource::String::ImportSearchFailed << ' ' << packageRequest.Id << std::endl;

                    // Keep searching for the remaining packages and only fail at the end.
                    foundAll = false;
                    continue;
                }

                packagesToInstall.push_back(std::move(searchContext.Get<Execution::Data::PackageVersion>()));
            }

            sources.push_back(std::move(source));
        }

        if (!foundAll)
        {
            // TODO: Set better error; report; log
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
        }

        context.Add<Execution::Data::PackagesToInstall>(std::move(packagesToInstall));
        context.Add<Execution::Data::Sources>(std::move(sources));
    }
}