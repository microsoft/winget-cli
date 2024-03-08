// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "ImportExportFlow.h"
#include "UpdateFlow.h"
#include "PackageCollection.h"
#include "DependenciesFlow.h"
#include "WorkflowBase.h"
#include <winget/RepositorySearch.h>
#include <winget/Runtime.h>
#include <winget/PackageVersionSelection.h>

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

        SourceDetails GetSourceDetails(const Repository::Source& source)
        {
            return source.GetDetails();
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
            std::shared_ptr<ICompositePackage> package,
            Utility::LocIndView version,
            Utility::LocIndView channel,
            bool checkVersion)
        {
            std::shared_ptr<IPackageVersionCollection> availableVersions = GetAvailableVersionsForInstalledVersion(package);

            if (!checkVersion)
            {
                return availableVersions->GetLatestVersion();
            }

            auto availablePackageVersion = availableVersions->GetVersion({ "", version, channel });
            if (!availablePackageVersion)
            {
                availablePackageVersion = availableVersions->GetLatestVersion();
                if (availablePackageVersion)
                {
                    // Warn installed version is not available.
                    AICLI_LOG(
                        CLI,
                        Info,
                        << "Installed package version is not available."
                        << " Package Id [" << availablePackageVersion->GetProperty(PackageVersionProperty::Id) << "], Version [" << version << "], Channel [" << channel << "]"
                        << ". Found Version [" << availablePackageVersion->GetProperty(PackageVersionProperty::Version) << "], Channel [" << availablePackageVersion->GetProperty(PackageVersionProperty::Version) << "]");
                    context.Reporter.Warn() << Resource::String::InstalledPackageVersionNotAvailable(availablePackageVersion->GetProperty(PackageVersionProperty::Id), version, channel) << std::endl;
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
            auto installedPackageVersion = GetInstalledVersion(packageMatch.Package);
            auto version = installedPackageVersion->GetProperty(PackageVersionProperty::Version);
            auto channel = installedPackageVersion->GetProperty(PackageVersionProperty::Channel);

            // Find an available version of this package to determine its source.
            auto availablePackageVersion = GetAvailableVersionForInstalledPackage(context, packageMatch.Package, Utility::LocIndView{ version }, Utility::LocIndView{ channel }, includeVersions);
            if (!availablePackageVersion)
            {
                // Report package not found and move to next package.
                AICLI_LOG(CLI, Warning, << "No available version of package [" << installedPackageVersion->GetProperty(PackageVersionProperty::Name) << "] was found to export");
                context.Reporter.Warn() << Resource::String::InstalledPackageNotAvailable(installedPackageVersion->GetProperty(PackageVersionProperty::Name)) << std::endl;
                continue;
            }

            const auto& sourceDetails = availablePackageVersion->GetSource().GetDetails();
            AICLI_LOG(CLI, Info,
                << "Installed package is available. Package Id [" << availablePackageVersion->GetProperty(PackageVersionProperty::Id) << "], Source [" << sourceDetails.Identifier << "]");

            if (!availablePackageVersion->GetManifest().DefaultLocalization.Get<Manifest::Localization::Agreements>().empty())
            {
                // Report that the package requires accepting license terms
                AICLI_LOG(CLI, Warning, << "Package [" << installedPackageVersion->GetProperty(PackageVersionProperty::Name) << "] requires license agreement to install");
                context.Reporter.Warn() << Resource::String::ExportedPackageRequiresLicenseAgreement(installedPackageVersion->GetProperty(PackageVersionProperty::Name)) << std::endl;
            }

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
        std::ifstream importFile(Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::ImportFile)));
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
        auto availableSources = Repository::Source::GetCurrentSources();
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
                context.Reporter.Warn()
                    << Resource::String::ImportSourceNotInstalled(Utility::LocIndView{ requiredSource.Details.Name })
                    << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            }

            context << Workflow::OpenNamedSourceForSources(requiredSource.Details.Name);
            if (context.IsTerminated())
            {
                return;
            }
        }
    }

    void GetSearchRequestsForImport(Execution::Context& context)
    {
        const auto& sources = context.Get<Execution::Data::Sources>();
        std::vector<std::unique_ptr<Execution::Context>> packageSubContexts;

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
            Repository::Source source{ context.Get<Execution::Data::Source>(), *sourceItr, CompositeSearchBehavior::AllPackages };
            AICLI_LOG(CLI, Info, << "Identifying packages requested from source [" << requiredSource.Details.Identifier << "]");
            for (const auto& packageRequest : requiredSource.Packages)
            {
                AICLI_LOG(CLI, Info, << "Searching for package [" << packageRequest.Id << "]");

                // Search for the current package
                SearchRequest searchRequest;
                searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, packageRequest.Id.get()));

                auto searchContextPtr = context.CreateSubContext();
                Execution::Context& searchContext = *searchContextPtr;
                auto previousThreadGlobals = searchContext.SetForCurrentThread();

                searchContext.Add<Execution::Data::Source>(source);
                searchContext.Add<Execution::Data::SearchRequest>(std::move(searchRequest));

                if (packageRequest.Scope != Manifest::ScopeEnum::Unknown)
                {
                    // TODO: In the future, it would be better to not have to convert back and forth from a string
                    searchContext.Args.AddArg(Execution::Args::Type::InstallScope, ScopeToString(packageRequest.Scope));
                }

                auto versionString = packageRequest.VersionAndChannel.GetVersion().ToString();
                if (!versionString.empty())
                {
                    searchContext.Args.AddArg(Execution::Args::Type::Version, versionString);
                }

                auto channelString = packageRequest.VersionAndChannel.GetChannel().ToString();
                if (!channelString.empty())
                {
                    searchContext.Args.AddArg(Execution::Args::Type::Channel, channelString);
                }

                packageSubContexts.emplace_back(std::move(searchContextPtr));
            }
        }

        context.Add<Execution::Data::PackageSubContexts>(std::move(packageSubContexts));
    }

    void InstallImportedPackages(Execution::Context& context)
    {
        // Inform all dependencies here. During SubContexts processing, dependencies are ignored.
        auto& packageSubContexts = context.Get<Execution::Data::PackageSubContexts>();
        Manifest::DependencyList allDependencies;
        for (auto& packageContext : packageSubContexts)
        {
            allDependencies.Add(packageContext->Get<Execution::Data::Installer>().value().Dependencies);
        }
        context.Add<Execution::Data::Dependencies>(allDependencies);

        context <<
            Workflow::ReportDependencies(Resource::String::ImportCommandReportDependencies) <<
            Workflow::ProcessMultiplePackages(
                Resource::String::ImportCommandReportDependencies, APPINSTALLER_CLI_ERROR_IMPORT_INSTALL_FAILED, {}, true, true);

        if (context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_IMPORT_INSTALL_FAILED)
        {
            context.Reporter.Error() << Resource::String::ImportInstallFailed << std::endl;
        }
    }
}
