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
        auto installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
        Utility::Version installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        ManifestComparator manifestComparator(context.Args, installedPackage->GetMetadata());
        bool updateFound = false;

        // The version keys should have already been sorted by version
        const auto& versionKeys = m_package.GetAvailableVersionKeys();
        for (const auto& key : versionKeys)
        {
            // Check Update Version
            if (IsUpdateVersionApplicable(installedVersion, Utility::Version(key.Version)))
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
        for (const auto& match : matches)
        {
            // We want to do best effort to update all applicable updates regardless on previous update failure
            auto updateContextPtr = context.Clone();
            Execution::Context& updateContext = *updateContextPtr;
            updateContext.Reporter.Info() << std::endl;

            updateContext.Add<Execution::Data::InstalledPackageVersion>(match.Package->GetInstalledVersion());

            updateContext <<
                SelectLatestApplicableUpdate(*(match.Package)) <<
                ShowInstallationDisclaimer <<
                DownloadInstaller <<
                ExecuteInstaller <<
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