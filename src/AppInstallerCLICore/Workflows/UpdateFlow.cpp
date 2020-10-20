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
        bool IsUpdateVersionApplicable(Execution::Context& context, const Utility::Version& updateVersion)
        {
            const auto& installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
            const auto& installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));

            bool updateApplicable = false;
            if (updateVersion > installedVersion)
            {
                updateApplicable = true;
            }
            else if (updateVersion == installedVersion)
            {
                // If installer type is MSStore, we'll let Store api to handle updates later
                const auto& installationMetadata = installedPackage->GetInstallationMetadata();
                auto installerTypeItr = installationMetadata.find(s_InstallationMetadata_Key_InstallerType);
                if (installerTypeItr != installationMetadata.end() &&
                    Manifest::ManifestInstaller::InstallerTypeEnum::MSStore == Manifest::ManifestInstaller::ConvertToInstallerTypeEnum(installerTypeItr->second))
                {
                    updateApplicable = true;
                }
            }

            return updateApplicable;
        }
    }

    void SelectLatestApplicableUpdate::operator()(Execution::Context& context) const
    {
        const auto& installationMetadata = context.Get<Execution::Data::InstalledPackageVersion>()->GetInstallationMetadata();
        ManifestComparator manifestComparator(context.Args, installationMetadata);
        bool updateFound = false;

        // The version keys should have already been sorted by version
        const auto& versionKeys = m_package.GetAvailableVersionKeys();
        for (const auto& key : versionKeys)
        {
            // Check Update Version
            if (IsUpdateVersionApplicable(context, Utility::Version(key.Version)))
            {
                auto manifest = m_package.GetAvailableVersion(key)->GetManifest();

                // Check MinOSVersion
                if (!manifest.MinOSVersion.empty() &&
                    !Runtime::IsCurrentOSVersionGreaterThanOrEqual(Utility::Version(manifest.MinOSVersion)))
                {
                    continue;
                }

                // Check applicable Installer
                auto installer = manifestComparator.GetPreferredInstaller(manifest);
                if (!installer.has_value())
                {
                    continue;
                }

                // Since we already did installer selection, just populate the context Data
                context.Add<Execution::Data::Manifest>(std::move(manifest));
                context.Add<Execution::Data::Installer>(std::move(installer));

                updateFound = true;
            }
            else
            {
                // Any following versions are not applicable
                break;
            }
        }

        if (!updateFound)
        {
            context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
        }
    }

    void EnsureUpdateVersionApplicable(Execution::Context& context)
    {
        Utility::Version updateVersion(context.Get<Execution::Data::Manifest>().Version);

        if (!IsUpdateVersionApplicable(context, updateVersion))
        {
            context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
        }
    }

    void UpdateAllApplicable(Execution::Context& context)
    {
        const auto& matches = context.Get<Execution::Data::SearchResult>().Matches;
        bool updateAllHasFailure = false;
        for (const auto& match : matches)
        {
            Logging::SubExecutionTelemetryScope subExecution;

            // We want to do best effort to update all applicable updates regardless on previous update failure
            auto updateContextPtr = context.Clone();
            Execution::Context& updateContext = *updateContextPtr;
            updateContext.Reporter.Info() << std::endl;

            updateContext.Add<Execution::Data::InstalledPackageVersion>(match.Package->GetInstalledVersion());

            updateContext <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                SelectLatestApplicableUpdate(*(match.Package)) <<
                ShowInstallationDisclaimer <<
                Workflow::ReportExecutionStage(ExecutionStage::Download) <<
                DownloadInstaller <<
                Workflow::ReportExecutionStage(ExecutionStage::Execution) <<
                ExecuteInstaller <<
                Workflow::ReportExecutionStage(ExecutionStage::PostExecution) <<
                RemoveInstaller;

            if (updateContext.GetTerminationHR() != S_OK &&
                updateContext.GetTerminationHR() != APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
            {
                updateAllHasFailure = true;
            }
        }

        if (updateAllHasFailure)
        {
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE);
        }
    }
}