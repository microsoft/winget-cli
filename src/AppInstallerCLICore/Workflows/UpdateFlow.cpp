// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "WorkflowBase.h"
#include "InstallFlow.h"
#include "UpdateFlow.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        bool IsUpdateVersionApplicable(const Utility::Version& installedVersion, const Utility::Version& updateVersion)
        {
            return (installedVersion < updateVersion || updateVersion.IsLatest());
        }
    }

    void SelectLatestApplicableUpdate::operator()(Execution::Context& context) const
    {
        auto package = context.Get<Execution::Data::Package>();
        auto installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
        Utility::Version installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        ManifestComparator manifestComparator(context.Args, installedPackage->GetMetadata());
        bool updateFound = false;

        // The version keys should have already been sorted by version
        const auto& versionKeys = package->GetAvailableVersionKeys();
        for (const auto& key : versionKeys)
        {
            // Check Update Version
            if (IsUpdateVersionApplicable(installedVersion, Utility::Version(key.Version)))
            {
                auto packageVersion = package->GetAvailableVersion(key);
                auto manifest = packageVersion->GetManifest();

                // Check applicable Installer
                auto installer = manifestComparator.GetPreferredInstaller(manifest);
                if (!installer.has_value())
                {
                    continue;
                }

                // Since we already did installer selection, just populate the context Data
                manifest.ApplyLocale();
                context.Add<Execution::Data::Manifest>(std::move(manifest));
                context.Add<Execution::Data::PackageVersion>(std::move(packageVersion));
                context.Add<Execution::Data::Installer>(std::move(installer));

                updateFound = true;
                break;
            }
            else
            {
                // Any following versions are not applicable
                break;
            }
        }

        if (!updateFound)
        {
            if (m_reportUpdateNotFound)
            {
                context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl;
            }
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
        }
    }

    void EnsureUpdateVersionApplicable(Execution::Context& context)
    {
        auto installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
        Utility::Version installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        Utility::Version updateVersion(context.Get<Execution::Data::Manifest>().Version);

        if (!IsUpdateVersionApplicable(installedVersion, updateVersion))
        {
            context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
        }
    }

    void UpdateAllApplicable(Execution::Context& context)
    {
        const auto& matches = context.Get<Execution::Data::SearchResult>().Matches;
        bool updateAllHasFailure = false;
        bool updateAllFoundUpdate = false;

        for (const auto& match : matches)
        {
            Logging::SubExecutionTelemetryScope subExecution;

            // We want to do best effort to update all applicable updates regardless on previous update failure
            auto updateContextPtr = context.Clone();
            Execution::Context& updateContext = *updateContextPtr;

            updateContext.Add<Execution::Data::Package>(match.Package);

            updateContext <<
                Workflow::GetInstalledPackageVersion <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                SelectLatestApplicableUpdate(false);

            if (updateContext.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
            {
                continue;
            }

            updateAllFoundUpdate = true;

            updateContext << InstallPackageInstaller;

            updateContext.Reporter.Info() << std::endl;

            // msstore update might still terminate with APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE
            if (updateContext.GetTerminationHR() != S_OK &&
                updateContext.GetTerminationHR() != APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
            {
                updateAllHasFailure = true;
            }
        }

        if (!updateAllFoundUpdate)
        {
            context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl;
        }

        if (updateAllHasFailure)
        {
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE);
        }
    }
}