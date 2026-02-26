// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "MultiQueryFlow.h"
#include "UpdateFlow.h"

using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        Utility::LocIndString GetPackageStringFromSearchRequest(const SearchRequest& searchRequest)
        {
            if (searchRequest.Query)
            {
                return Utility::LocIndString{ searchRequest.Query->Value };
            }

            if (!searchRequest.Inclusions.empty())
            {
                return Utility::LocIndString{ searchRequest.Inclusions[0].Value };
            }

            if (!searchRequest.Filters.empty())
            {
                return Utility::LocIndString{ searchRequest.Filters[0].Value };
            }

            return ""_lis;
        }
    }

    void GetMultiSearchRequests(Execution::Context& context)
    {
        std::vector<std::unique_ptr<Execution::Context>> packageSubContexts;
        auto& source = context.Get<Execution::Data::Source>();
        for (const auto& query : *context.Args.GetArgs(Execution::Args::Type::MultiQuery))
        {
            auto searchContextPtr = context.CreateSubContext();
            Execution::Context& searchContext = *searchContextPtr;
            auto previousThreadGlobals = searchContext.SetForCurrentThread();

            searchContext.Add<Execution::Data::Source>(source);
            searchContext.Args.AddArg(Execution::Args::Type::Query, query);

            AICLI_LOG(CLI, Info, << "Creating search query for package [" << query << "]");
            searchContext << GetSearchRequestForSingle;

            packageSubContexts.emplace_back(std::move(searchContextPtr));
        }

        context.Add<Execution::Data::PackageSubContexts>(std::move(packageSubContexts));
    }

    void SearchSubContextsForSingle::operator()(Execution::Context& context) const
    {
        std::vector<std::unique_ptr<Execution::Context>> packageSubContexts;
        bool foundAll = true;

        for (auto& searchContextPtr : context.Get<Execution::Data::PackageSubContexts>())
        {
            auto& searchContext = *searchContextPtr;
            SearchRequest searchRequest = searchContext.Get<Execution::Data::SearchRequest>();
            searchContext.Add<Execution::Data::SearchResult>(searchContext.Get<Execution::Data::Source>().Search(searchRequest));

            switch (m_operationType)
            {
            case OperationType::Install:
            case OperationType::Upgrade:
                searchContext << Workflow::SelectSinglePackageVersionForInstallOrUpgrade(m_operationType);
                break;
            case OperationType::Uninstall:
                searchContext <<
                    Workflow::HandleSearchResultFailures <<
                    Workflow::EnsureOneMatchFromSearchResult(m_operationType);
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            if (searchContext.IsTerminated())
            {
                if (context.IsTerminated() && context.GetTerminationHR() == E_ABORT)
                {
                    // This means that the subcontext being terminated is due to an overall abort
                    context.Reporter.Info() << Resource::String::Cancelled << std::endl;
                    return;
                }
                else
                {
                    // We already reported the error from the sub-context, but we repeat it here because
                    // for multi-queries we can a bit more verbose as the queries here are easier to report.
                    auto packageString = GetPackageStringFromSearchRequest(searchRequest);
                    auto searchTerminationHR = searchContext.GetTerminationHR();
                    if (searchTerminationHR == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE ||
                        searchTerminationHR == APPINSTALLER_CLI_ERROR_PACKAGE_ALREADY_INSTALLED)
                    {
                        AICLI_LOG(CLI, Info, << "Package is already installed: [" << packageString << "]");
                        context.Reporter.Info() << Resource::String::MultiQueryPackageAlreadyInstalled(packageString) << std::endl;
                        continue;
                    }
                    else
                    {
                        if (searchTerminationHR == APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND)
                        {
                            AICLI_LOG(CLI, Info, << "Package not found for query: [" << packageString << "]");
                            context.Reporter.Warn() << Resource::String::MultiQueryPackageNotFound(packageString) << std::endl;
                        }
                        else if (searchTerminationHR == APPINSTALLER_CLI_ERROR_MULTIPLE_APPLICATIONS_FOUND)
                        {
                            AICLI_LOG(CLI, Info, << "Multiple packages found for query: [" << packageString << "]");
                            context.Reporter.Warn() << Resource::String::MultiQuerySearchFoundMultiple(packageString) << std::endl;
                        }
                        else
                        {
                            AICLI_LOG(CLI, Info, << "Search failed for query: [" << packageString << "]");
                            context.Reporter.Info() << Resource::String::MultiQuerySearchFailed(packageString) << std::endl;
                        }

                        // Keep searching for the remaining packages and only fail at the end.
                        foundAll = false;
                        continue;
                    }
                }
            }

            packageSubContexts.emplace_back(std::move(searchContextPtr));
        }

        if (!foundAll)
        {
            AICLI_LOG(CLI, Info, << "Not all queries returned one result");
            if (context.Args.Contains(Execution::Args::Type::IgnoreUnavailable))
            {
                AICLI_LOG(CLI, Info, << "Ignoring unavailable packages due to command line argument");
            }
            else
            {
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_NOT_ALL_QUERIES_FOUND_SINGLE);
            }
        }

        context.Add<Execution::Data::PackageSubContexts>(std::move(packageSubContexts));
    }
}
