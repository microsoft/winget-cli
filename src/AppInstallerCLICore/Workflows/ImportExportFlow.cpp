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

    namespace
    {
        SourceDetails GetSourceDetails(const SourceDetails& source)
        {
            return source;
        }

        SourceDetails GetSourceDetails(const PackageCollection::Source& source)
        {
            return source.Details;
        }

        SourceDetails GetSourceDetails(const std::shared_ptr<ISource>& source)
        {
            return source->GetDetails();
        }

        // Creates a predicate that determines whether a source matches a description in a SourceDetails.
        template<class T>
        std::function<bool(const T&)> GetSourceDetailsEquivalencePredicate(const SourceDetails& details)
        {
            return [&](const T& source)
            {
                SourceDetails sourceDetails = GetSourceDetails(source);
                return sourceDetails.Type == details.Type && sourceDetails.Identifier == details.Identifier;
            };
        }

        // Finds a source equivalent to the one specified.
        template<class T>
        typename std::vector<T>::const_iterator FindSource(const std::vector<T>& sources, const SourceDetails& details)
        {
            return std::find_if(sources.begin(), sources.end(), GetSourceDetailsEquivalencePredicate<T>(details));
        }

        // Finds a source equivalent to the one specified.
        template<class T>
        typename std::vector<T>::iterator FindSource(std::vector<T>& sources, const SourceDetails& details)
        {
            return std::find_if(sources.begin(), sources.end(), GetSourceDetailsEquivalencePredicate<T>(details));
        }

        // Gets the available version of an installed package.
        // If requested, checks that the installed version is available and reports a warning if it is not.
        std::shared_ptr<IPackageVersion> GetAvailableVersionForInstalledPackage(
            Execution::Context& context,
            std::shared_ptr<IPackage> package,
            std::string_view version,
            std::string_view channel,
            bool checkVersion)
        {
            if (!checkVersion)
            {
                return package->GetLatestAvailableVersion();
            }

            auto availablePackageVersion = package->GetAvailableVersion({ "", version, channel });
            if (!availablePackageVersion)
            {
                availablePackageVersion = package->GetLatestAvailableVersion();
                if (availablePackageVersion)
                {
                    // Warn installed version is not available.
                    AICLI_LOG(
                        CLI,
                        Info,
                        << "Installed package version is not available."
                        << " Package Id [" << availablePackageVersion->GetProperty(PackageVersionProperty::Id) << "], Version [" << version << "], Channel [" << channel << "]"
                        << ". Found Version [" << availablePackageVersion->GetProperty(PackageVersionProperty::Version) << "], Channel [" << availablePackageVersion->GetProperty(PackageVersionProperty::Version) << "]");
                    context.Reporter.Warn()
                        << Resource::String::InstalledPackageVersionNotAvailable
                        << ' ' << availablePackageVersion->GetProperty(PackageVersionProperty::Id)
                        << ' ' << version << ' ' << channel << std::endl;
                }
            }

            return availablePackageVersion;
        }
    }

    void SelectVersionsToExport(Execution::Context& context)
    {
        const auto& searchResult = context.Get<Execution::Data::SearchResult>();
        const bool includeVersions = context.Args.Contains(Execution::Args::Type::IncludeVersions);
        PackageCollection exportedPackages;
        exportedPackages.ClientVersion = Runtime::GetClientVersion().get();
        auto& exportedSources = exportedPackages.Sources;
        for (const auto& packageMatch : searchResult.Matches)
        {
            auto installedPackageVersion = packageMatch.Package->GetInstalledVersion();
            auto version = installedPackageVersion->GetProperty(PackageVersionProperty::Version);
            auto channel = installedPackageVersion->GetProperty(PackageVersionProperty::Channel);

            // Find an available version of this package to determine its source.
            auto availablePackageVersion = GetAvailableVersionForInstalledPackage(context, packageMatch.Package, version, channel, includeVersions);
            if (!availablePackageVersion)
            {
                // Report package not found and move to next package.
                AICLI_LOG(CLI, Warning, << "No available version of package [" << installedPackageVersion->GetProperty(PackageVersionProperty::Name) << "] was found to export");
                context.Reporter.Warn() << Resource::String::InstalledPackageNotAvailable << ' ' << installedPackageVersion->GetProperty(PackageVersionProperty::Name) << std::endl;
                continue;
            }

            const auto& sourceDetails = availablePackageVersion->GetSource()->GetDetails();
            AICLI_LOG(CLI, Info,
                << "Installed package is available. Package Id [" << availablePackageVersion->GetProperty(PackageVersionProperty::Id) << "], Source [" << sourceDetails.Identifier << "]");

            // Find the exported source for this package
            auto sourceItr = FindSource(exportedSources, sourceDetails);
            if (sourceItr == exportedSources.end())
            {
                exportedSources.emplace_back(sourceDetails);
                sourceItr = std::prev(exportedSources.end());
            }

            // Take the Id from the available package because that is the one used in the source,
            // but take the exported version from the installed package if needed.
            if (includeVersions)
            {
                sourceItr->Packages.emplace_back(
                    availablePackageVersion->GetProperty(PackageVersionProperty::Id),
                    version.get(),
                    channel.get());
            }
            else
            {
                sourceItr->Packages.emplace_back(availablePackageVersion->GetProperty(PackageVersionProperty::Id));
            }
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

        PackagesJson::ParseResult parseResult = PackagesJson::TryParseJson(jsonRoot);
        if (parseResult.Result != PackagesJson::ParseResult::Type::Success)
        {
            context.Reporter.Error() << Resource::String::InvalidJsonFile << std::endl;
            if (parseResult.Result == PackagesJson::ParseResult::Type::MissingSchema ||
                parseResult.Result == PackagesJson::ParseResult::Type::UnrecognizedSchema)
            {
                context.Reporter.Error() << Resource::String::ImportFileHasInvalidSchema << std::endl;
            }
            else if (parseResult.Result == PackagesJson::ParseResult::Type::SchemaValidationFailed)
            {
                context.Reporter.Error() << parseResult.Errors << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
        }

        PackageCollection& packages = parseResult.Packages;
        if (packages.Sources.empty())
        {
            AICLI_LOG(CLI, Warning, << "No packages to install");
            context.Reporter.Info() << Resource::String::NoPackagesFoundInImportFile << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
        }

        if (context.Args.Contains(Execution::Args::Type::IgnoreVersions))
        {
            // Strip out all the version information as we don't need it.
            for (auto& source : packages.Sources)
            {
                for (auto& package : source.Packages)
                {
                    package.VersionAndChannel = {};
                }
            }
        }

        context.Add<Execution::Data::PackageCollection>(std::move(packages));
    }

    void OpenSourcesForImport(Execution::Context& context)
    {
        auto availableSources = Repository::GetSources();
        for (auto& requiredSource : context.Get<Execution::Data::PackageCollection>().Sources)
        {
            // Find the installed source matching the one described in the collection.
            AICLI_LOG(CLI, Info, << "Looking for source [" << requiredSource.Details.Identifier << "]");
            auto matchingSource = FindSource(availableSources, requiredSource.Details);
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

            context << Workflow::OpenNamedSourceForSources(requiredSource.Details.Name);
            if (context.IsTerminated())
            {
                return;
            }
        }
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
            auto sourceItr = FindSource(sources, requiredSource.Details);
            if (sourceItr == sources.end())
            {
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR);
            }

            // Search for all the packages in the source.
            // Each search is done in a sub context to search everything regardless of previous failures.
            auto source = Repository::CreateCompositeSource(context.Get<Execution::Data::Source>(), *sourceItr, CompositeSearchBehavior::AllPackages);
            AICLI_LOG(CLI, Info, << "Searching for packages requested from source [" << requiredSource.Details.Identifier << "]");
            for (const auto& packageRequest : requiredSource.Packages)
            {
                Logging::SubExecutionTelemetryScope subExecution;
                AICLI_LOG(CLI, Info, << "Searching for package [" << packageRequest.Id << "]");

                // Search for the current package
                SearchRequest searchRequest;
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, packageRequest.Id.get()));

                auto searchContextPtr = context.Clone();
                Execution::Context& searchContext = *searchContextPtr;
                searchContext.Add<Execution::Data::Source>(source);
                searchContext.Add<Execution::Data::SearchResult>(source->Search(searchRequest));

                // Find the single version we want is available
                searchContext <<
                    Workflow::EnsureOneMatchFromSearchResult(false) <<
                    Workflow::GetManifestWithVersionFromPackage(packageRequest.VersionAndChannel) <<
                    Workflow::GetInstalledPackageVersion;

                if (searchContext.Contains(Execution::Data::InstalledPackageVersion) && searchContext.Get<Execution::Data::InstalledPackageVersion>())
                {
                    searchContext << Workflow::EnsureUpdateVersionApplicable;
                }

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
                        AICLI_LOG(CLI, Info, << "Package not found for import: [" << packageRequest.Id << "], Version " << packageRequest.VersionAndChannel.ToString());
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
            if (context.Args.Contains(Execution::Args::Type::IgnoreUnavailable))
            {
                AICLI_LOG(CLI, Info, << "Ignoring unavailable packages due to command line argument");
            }
            else
            {
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND);
            }
        }

        context.Add<Execution::Data::PackagesToInstall>(std::move(packagesToInstall));
    }
}
