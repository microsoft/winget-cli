// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallFlow.h"
#include "ImportExportFlow.h"
#include "PackageCollection.h"
#include "WorkflowBase.h"
#include "CompositeSource.h"

namespace AppInstaller::CLI::Workflow
{
    void Export(Execution::Context& context)
    {
        std::filesystem::path outputFile{ context.Args.GetArg(Execution::Args::Type::OutputFile) };
        auto packages = ConvertSearchResultToPackageRequests(context.Get<Execution::Data::SearchResult>());
        std::ofstream{ outputFile } << ConvertPackageRequestsToJson(packages);
    }

    void ReadImportFile(Execution::Context& context)
    {
        std::filesystem::path importFile{ context.Args.GetArg(Execution::Args::Type::ImportFile) };
        Json::Value jsonRoot = {};
        std::ifstream{ importFile } >> jsonRoot;
        auto packages = ParsePackageCollection(jsonRoot);
        context.Add<Execution::Data::PackageRequests>(packages);

        if (packages.empty())
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

        for (auto& requiredSource : context.Get<Execution::Data::PackageRequests>())
        {
            // Match required sources with the available sources by their arguments,
            // as they may have been added with a different name.
            std::optional<SourceDetails> matchingSource = {};
            for (auto& availableSource : availableSources)
            {
                if (availableSource.Arg == requiredSource.SourceArg.get())
                {
                    matchingSource = availableSource;
                    break;
                }
            }

            if (matchingSource.has_value())
            {
                requiredSource.SourceName = Utility::LocIndString{ matchingSource.value().Name };
            }
            else
            {
                // TODO: Add option for ignoring/installing missing sources?
                AICLI_LOG(CLI, Error, << "Missing required source: " << requiredSource.SourceName);
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
            }

            // Search for all the packages in the source
            Logging::SubExecutionTelemetryScope subExecution;

            // We want to do best effort to install all packages regardless of previous failures
            auto searchContextPtr = context.Clone();
            Execution::Context& searchContext = *searchContextPtr;

            searchContext << OpenNamedSource(requiredSource.SourceName);
            auto source = searchContext.Get<Execution::Data::Source>();
            aggregatedSource->AddAvailableSource(source);
            for (const auto& packageRequest : requiredSource.Packages)
            {
                // TODO: Case insensitive?
                MatchType matchType = MatchType::Exact;

                SearchRequest searchRequest;
                searchRequest.Inclusions.emplace_back(PackageMatchFilter(PackageMatchField::Id, matchType, packageRequest.Id));

                searchContext.Add<Execution::Data::SearchResult>(source->Search(searchRequest));
                searchContext << EnsureOneMatchFromSearchResult(false);

                PackageVersionKey requestedVersion
                {
                    "",
                    packageRequest.VersionAndChannel.GetVersion().ToString(),
                    packageRequest.VersionAndChannel.GetChannel().ToString(),
                };

                // TODO: handle unavailable version
                packagesToInstall.push_back(searchContext.Get<Execution::Data::Package>()->GetAvailableVersion(requestedVersion));
            }
        }

        context.Add<Execution::Data::PackagesToInstall>(std::move(packagesToInstall));
        context.Add<Execution::Data::Source>(std::move(aggregatedSource));
    }
}