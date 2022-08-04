// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "WorkflowBase.h"
#include "DependenciesFlow.h"
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
            return installedVersion < updateVersion;
        }

        void AddToPackagesToInstallIfNotPresent(std::vector<std::unique_ptr<Execution::Context>>& packagesToInstall, std::unique_ptr<Execution::Context> packageContext)
        {
            for (auto const& existing : packagesToInstall)
            {
                if (existing->Get<Execution::Data::Manifest>().Id == packageContext->Get<Execution::Data::Manifest>().Id &&
                    existing->Get<Execution::Data::Manifest>().Version == packageContext->Get<Execution::Data::Manifest>().Version &&
                    existing->Get<Execution::Data::PackageVersion>()->GetProperty(PackageVersionProperty::SourceIdentifier) == packageContext->Get<Execution::Data::PackageVersion>()->GetProperty(PackageVersionProperty::SourceIdentifier))
                {
                    return;
                }
            }

            packagesToInstall.emplace_back(std::move(packageContext));
        }
    }

    void SelectLatestApplicableUpdate::operator()(Execution::Context& context) const
    {
        auto package = context.Get<Execution::Data::Package>();
        auto installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
        Utility::Version installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        ManifestComparator manifestComparator(context, installedPackage->GetMetadata());
        bool updateFound = false;
        bool installedTypeInapplicable = false;

        if (installedVersion.IsUnknown() && !context.Args.Contains(Execution::Args::Type::IncludeUnknown))
        {
            // the package has an unknown version and the user did not request to upgrade it anyway.
            if (m_reportUpdateNotFound)
            {
                context.Reporter.Info() << Resource::String::UpgradeUnknownVersionExplanation << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
        }

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
                auto [installer, inapplicabilities] = manifestComparator.GetPreferredInstaller(manifest);
                if (!installer.has_value())
                {
                    // If there is at least one installer whose only reason is InstalledType.
                    auto onlyInstalledType = std::find(inapplicabilities.begin(), inapplicabilities.end(), InapplicabilityFlags::InstalledType);
                    if (onlyInstalledType != inapplicabilities.end())
                    {
                        installedTypeInapplicable = true;
                    }

                    continue;
                }

                Logging::Telemetry().LogSelectedInstaller(
                    static_cast<int>(installer->Arch),
                    installer->Url,
                    Manifest::InstallerTypeToString(installer->EffectiveInstallerType()),
                    Manifest::ScopeToString(installer->Scope),
                    installer->Locale);

                Logging::Telemetry().LogManifestFields(
                    manifest.Id,
                    manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>(),
                    manifest.Version);

                // Since we already did installer selection, just populate the context Data
                manifest.ApplyLocale(installer->Locale);
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
                if (installedTypeInapplicable)
                {
                    context.Reporter.Info() << Resource::String::UpgradeDifferentInstallTechnologyInNewerVersions << std::endl;
                }
                else
                {
                    context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl;
                }
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
        std::vector<std::unique_ptr<Execution::Context>> packagesToInstall;
        bool updateAllFoundUpdate = false;
        int packagesWithUnknownVersionSkipped = 0;
        int packagesThatRequireExplicitSkipped = 0;

        for (const auto& match : matches)
        {
            // We want to do best effort to update all applicable updates regardless on previous update failure
            auto updateContextPtr = context.CreateSubContext();
            Execution::Context& updateContext = *updateContextPtr;
            auto previousThreadGlobals = updateContext.SetForCurrentThread();
            auto installedVersion = match.Package->GetInstalledVersion();

            updateContext.Add<Execution::Data::Package>(match.Package);

            // Filter out packages with unknown installed versions
            if (context.Args.Contains(Execution::Args::Type::IncludeUnknown))
            {
                updateContext.Args.AddArg(Execution::Args::Type::IncludeUnknown);
            }
            else if (Utility::Version(installedVersion->GetProperty(PackageVersionProperty::Version)).IsUnknown())
            {
                // we don't know what the package's version is and the user didn't ask to upgrade it anyway.
                AICLI_LOG(CLI, Info, << "Skipping " << match.Package->GetProperty(PackageProperty::Id) << " as it has unknown installed version");
                ++packagesWithUnknownVersionSkipped;
                continue;
            }

            updateContext <<
                Workflow::GetInstalledPackageVersion <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                SelectLatestApplicableUpdate(false);

            if (updateContext.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
            {
                continue;
            }

            // Filter out packages that require explicit upgrades.
            // We require explicit upgrades only if the installed version is pinned,
            // either because it was manually pinned or because the manifest indicated
            // RequireExplicitUpgrade.
            // Note that this does not consider whether the update to be installed has
            // RequireExplicitUpgrade. While this has the downside of not working with
            // packages installed from another source, it ensures consistency with the
            // list of available updates (there we don't have the selected installer)
            // and at most we will update each package like this once.
            auto installedMetadata = updateContext.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata();
            auto pinnedState = ConvertToPackagePinnedStateEnum(installedMetadata[PackageVersionMetadata::PinnedState]);
            if (pinnedState != PackagePinnedState::NotPinned)
            {
                AICLI_LOG(CLI, Info, << "Skipping " << match.Package->GetProperty(PackageProperty::Id) << " as it requires explicit upgrade");
                ++packagesThatRequireExplicitSkipped;
                continue;
            }

            updateAllFoundUpdate = true;

            AddToPackagesToInstallIfNotPresent(packagesToInstall, std::move(updateContextPtr));
        }

        if (updateAllFoundUpdate)
        {
            context.Add<Execution::Data::PackagesToInstall>(std::move(packagesToInstall));
            context.Reporter.Info() << std::endl;
            context <<
                InstallMultiplePackages(
                    Resource::String::InstallAndUpgradeCommandsReportDependencies,
                    APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE,
                    { APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE });
        }

        if (packagesWithUnknownVersionSkipped > 0)
        {
            AICLI_LOG(CLI, Info, << packagesWithUnknownVersionSkipped << " package(s) skipped due to unknown installed version");
            context.Reporter.Info() << packagesWithUnknownVersionSkipped << " " << Resource::String::UpgradeUnknownVersionCount << std::endl;
        }

        if (packagesThatRequireExplicitSkipped > 0)
        {
            AICLI_LOG(CLI, Info, << packagesThatRequireExplicitSkipped << " package(s) skipped due to requiring explicit upgrade");
            context.Reporter.Info() << packagesThatRequireExplicitSkipped << " " << Resource::String::UpgradeRequireExplicitCount << std::endl;
        }
    }
}
