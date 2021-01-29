// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "ImportExportFlow.h"
#include "UpdateFlow.h"
#include "PackageCollection.h"
#include "WorkflowBase.h"
#include "AppInstallerRepositorySearch.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::Repository;

    void SelectVersionsToExport(Execution::Context& context)
    {
        const auto& searchResult = context.Get<Execution::Data::SearchResult>();
        PackageCollection exportedPackages;
        exportedPackages.ClientVersion = Runtime::GetClientVersion().get();
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
        std::ifstream importFile{ context.Args.GetArg(Execution::Args::Type::ImportFile) };
        THROW_LAST_ERROR_IF(importFile.fail());

        Json::Value jsonRoot;
        Json::CharReaderBuilder builder;
        Json::String errors;
        if (!Json::parseFromStream(builder, importFile, &jsonRoot, &errors))
        {
            AICLI_LOG(CLI, Error, << "Failed to read JSON: " << errors);
            context.Reporter.Error() << Resource::String::InvalidJsonFile << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
        }

        auto packages = PackagesJson::ParseJson(jsonRoot);
        if (!packages.has_value())
        {
            context.Reporter.Error() << Resource::String::InvalidJsonFile << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
        }

        if (packages.value().Sources.empty())
        {
            AICLI_LOG(CLI, Warning, << "No packages to install");
            context.Reporter.Info() << Resource::String::NoPackagesFoundInImportFile << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
        }

        context.Add<Execution::Data::PackageCollection>(packages.value());
    }

    void OpenSourcesForImport(Execution::Context& context)
    {
        auto availableSources = Repository::GetSources();

        // List of all the sources used for import.
        // Needed to keep all the source objects alive for install.
        std::vector<std::shared_ptr<ISource>> sources = {};

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

            context << Workflow::OpenNamedSource(requiredSource.Details.Name);
            if (context.IsTerminated())
            {
                return;
            }

            sources.push_back(context.Get<Execution::Data::Source>());
        }

        context.Add<Execution::Data::Sources>(std::move(sources));
    }

    void SearchPackagesForImport(Execution::Context& context)
    {
        const auto& sources = context.Get<Execution::Data::Sources>();
        std::vector<std::shared_ptr<IPackageVersion>> packagesToInstall = {};
        bool foundAll = true;

        // Look for the packages needed from each source independently.
        // If a package is available from multiple sources, this ensures we will get it from the right one.
        for (auto& requiredSource : context.Get<Execution::Data::PackageCollection>().Sources)
        {
            // Find the required source among the open sources. This must exist as we already found them.
            auto sourceItr = std::find_if(
                sources.begin(),
                sources.end(),
                [&](const std::shared_ptr<ISource>& s) { return s->GetIdentifier() == requiredSource.Details.Identifier; });
            if (sourceItr == sources.end())
            {
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
            }

            // Search for all the packages in the source.
            // Each search is done in a sub context to search everything regardless of previous failures.
            auto source = Repository::CreateCompositeSource(context.Get<Execution::Data::Source>(), *sourceItr);
            for (const auto& packageRequest : requiredSource.Packages)
            {
                Logging::SubExecutionTelemetryScope subExecution;

                // Search for the current package
                SearchRequest searchRequest;
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, packageRequest.Id));

                auto searchContextPtr = context.Clone();
                Execution::Context& searchContext = *searchContextPtr;
                searchContext.Add<Execution::Data::Source>(source);
                searchContext.Add<Execution::Data::SearchResult>(source->Search(searchRequest));

                Utility::VersionAndChannel versionAndChannel = {};
                if (context.Args.Contains(Execution::Args::Type::ExactVersions))
                {
                    versionAndChannel = packageRequest.VersionAndChannel;
                }

                // Find the single version we want is available
                searchContext <<
                    Workflow::EnsureOneMatchFromSearchResult(false) <<
                    Workflow::GetManifestWithVersionFromPackage(versionAndChannel) <<
                    Workflow::GetInstalledPackageVersion <<
                    Workflow::EnsureUpdateVersionApplicable;

                if (searchContext.IsTerminated())
                {
                    if (searchContext.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
                    {
                        AICLI_LOG(CLI, Info, << "Package is already installed: [" << packageRequest.Id << "]");
                        context.Reporter.Info() << Resource::String::ImportPackageAlreadyInstalled << ' ' << packageRequest.Id << std::endl;
                        continue;
                    }
                    else
                    {
                        AICLI_LOG(CLI, Info, << "Package not found for import: [" << packageRequest.Id << "], Version " << versionAndChannel.ToString());
                        context.Reporter.Info() << Resource::String::ImportSearchFailed << ' ' << packageRequest.Id << std::endl;

                        // Keep searching for the remaining packages and only fail at the end.
                        foundAll = false;
                        continue;
                    }
                }

                packagesToInstall.push_back(std::move(searchContext.Get<Execution::Data::PackageVersion>()));
            }
        }

        if (!foundAll)
        {
            AICLI_LOG(CLI, Info, << "Could not find one or more packages for import");
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND);
        }

        context.Add<Execution::Data::PackagesToInstall>(std::move(packagesToInstall));
    }
}