// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowBase.h"
#include "DependenciesFlow.h"
#include "InstallFlow.h"
#include "UpdateFlow.h"
#include <winget/ManifestComparator.h>
#include <winget/PinningData.h>
#include <winget/PackageVersionSelection.h>
#include <ctime>

using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Pinning;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        std::optional<std::chrono::system_clock::time_point> TryParseISO8601Date(std::string_view value)
        {
            // YYYY-MM-DD is the expected format
            if (value.size() < 10)
            {
                return {};
            }

            auto toInt = [](std::string_view sv) -> std::optional<int>
            {
                int result = 0;
                for (char c : sv)
                {
                    if (c < '0' || c > '9')
                    {
                        return {};
                    }
                    result = (result * 10) + (c - '0');
                }
                return result;
            };

            auto year = toInt(value.substr(0, 4));
            auto month = toInt(value.substr(5, 2));
            auto day = toInt(value.substr(8, 2));

            if (!year || !month || !day || value[4] != '-' || value[7] != '-')
            {
                return {};
            }

            if (*month < 1 || *month > 12 || *day < 1 || *day > 31)
            {
                return {};
            }

            std::tm tm{};
            tm.tm_year = *year - 1900;
            tm.tm_mon = *month - 1;
            tm.tm_mday = *day;
            tm.tm_hour = 0;
            tm.tm_min = 0;
            tm.tm_sec = 0;

            time_t utcTime = _mkgmtime(&tm);
            if (utcTime == static_cast<time_t>(-1))
            {
                return {};
            }

            return std::chrono::system_clock::from_time_t(utcTime);
        }

        bool IsUpgradeEligibleByDelay(std::string_view releaseDate, std::chrono::hours upgradeDelay)
        {
            if (upgradeDelay <= std::chrono::hours{ 0 })
            {
                return true;
            }

            auto parsed = TryParseISO8601Date(releaseDate);
            if (!parsed)
            {
                return false;
            }

            return (std::chrono::system_clock::now() - *parsed) >= upgradeDelay;
        }

        bool IsUpdateVersionAvailable(const Utility::Version& installedVersion, const Utility::Version& updateVersion)
        {
            return installedVersion < updateVersion;
        }

        void AddToPackageSubContextsIfNotPresent(std::vector<std::unique_ptr<Execution::Context>>& packageSubContexts, std::unique_ptr<Execution::Context> packageContext)
        {
            for (auto const& existing : packageSubContexts)
            {
                if (existing->Get<Execution::Data::Manifest>().Id == packageContext->Get<Execution::Data::Manifest>().Id &&
                    existing->Get<Execution::Data::Manifest>().Version == packageContext->Get<Execution::Data::Manifest>().Version &&
                    existing->Get<Execution::Data::PackageVersion>()->GetProperty(PackageVersionProperty::SourceIdentifier) == packageContext->Get<Execution::Data::PackageVersion>()->GetProperty(PackageVersionProperty::SourceIdentifier))
                {
                    return;
                }
            }

            packageSubContexts.emplace_back(std::move(packageContext));
        }
    }

    void SelectLatestApplicableVersion::operator()(Execution::Context& context) const
    {
        auto package = context.Get<Execution::Data::Package>();
        auto installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
        const bool reportVersionNotFound = m_isSinglePackage;

        bool isUpgrade = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerExecutionUseUpdate);;
        Utility::Version installedVersion;
        if (isUpgrade)
        {
            installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        }

        Manifest::ManifestComparator manifestComparator(GetManifestComparatorOptions(context, isUpgrade ? installedPackage->GetMetadata() : IPackageVersion::Metadata{}));
        bool versionFound = false;
        bool installedTypeInapplicable = false;
        bool packagePinned = false;

        if (isUpgrade && installedVersion.IsUnknown() && !context.Args.Contains(Execution::Args::Type::IncludeUnknown))
        {
            // the package has an unknown version and the user did not request to upgrade it anyway
            if (reportVersionNotFound)
            {
                context.Reporter.Info() << Resource::String::UpgradeUnknownVersionExplanation << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
        }

        // If we are updating a single package or we got the --include-pinned flag,
        // we include packages with Pinning pins
        const bool includePinned = m_isSinglePackage || context.Args.Contains(Execution::Args::Type::IncludePinned);

        PinningData pinningData{ PinningData::Disposition::ReadOnly };
        auto evaluator = pinningData.CreatePinStateEvaluator(includePinned ? PinBehavior::IncludePinned : PinBehavior::ConsiderPins, GetInstalledVersion(package));

        // The version keys should have already been sorted by version
        auto availableVersions = GetAvailableVersionsForInstalledVersion(package);
        const auto& versionKeys = availableVersions->GetVersionKeys();
        // Assume that no update versions are applicable
        bool upgradeVersionAvailable = false;
        for (const auto& key : versionKeys)
        {
            // Check Applicable Version
            if (!isUpgrade || IsUpdateVersionAvailable(installedVersion, Utility::Version(key.Version)))
            {
                // The only way to enter this portion of the statement with isUpgrade is if the version is available
                if (isUpgrade)
                {
                    AICLI_LOG(CLI, Verbose, << "Updating from [" << installedVersion.ToString() << "] to [" << key.Version << "]");
                    upgradeVersionAvailable = true;
                }

                auto packageVersion = availableVersions->GetVersion(key);

                // Check if the package is pinned
                PinType pinType = evaluator.EvaluatePinType(packageVersion);
                if (pinType != Pinning::PinType::Unknown)
                {
                    AICLI_LOG(CLI, Info, << "Package [" << package->GetProperty(PackageProperty::Id) << " with Version[" << key.Version << "] from Source[" << key.SourceId << "] has a Pin with type[" << ToString(pinType) << "]");
                    if (context.Args.Contains(Execution::Args::Type::Force))
                    {
                        AICLI_LOG(CLI, Info, << "Ignoring pin due to --force argument");
                    }
                    else
                    {
                        packagePinned = true;
                        continue;
                    }
                }

                auto manifest = packageVersion->GetManifest();

                // Check applicable Installer
                auto [installer, inapplicabilities] = manifestComparator.GetPreferredInstaller(manifest);
                if (!installer.has_value())
                {
                    // If there is at least one installer whose only reason is InstalledType.
                    auto onlyInstalledType = std::find(inapplicabilities.begin(), inapplicabilities.end(), Manifest::InapplicabilityFlags::InstalledType);
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

                versionFound = true;
                break;
            }
            else
            {
                // Any following versions are not applicable
                break;
            }
        }

        if (!versionFound)
        {
            if (reportVersionNotFound)
            {
                if (installedTypeInapplicable)
                {
                    context.Reporter.Info() << Resource::String::UpgradeDifferentInstallTechnologyInNewerVersions << std::endl;
                }
                else if (packagePinned)
                {
                    context.Reporter.Info() << Resource::String::UpgradeIsPinned << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PACKAGE_IS_PINNED);
                }
                else if (isUpgrade)
                {
                    if (!upgradeVersionAvailable)
                    {
                        // This is the case when no newer versions are available in a configured source
                        context.Reporter.Info() << Resource::String::UpdateNoPackagesFound << std::endl
                            << Resource::String::UpdateNoPackagesFoundReason << std::endl;
                    }
                    else
                    {
                        // This is the case when newer versions are available in a configured source, but none are applicable due to OS Version, user requirement, etc.
                        context.Reporter.Info() << Resource::String::UpdateNotApplicable << std::endl
                            << Resource::String::UpdateNotApplicableReason << std::endl;
                    }
                }
                else
                {
                    context.Reporter.Error() << Resource::String::NoApplicableInstallers << std::endl;
                }
            }
            if (isUpgrade && installedTypeInapplicable)
            {
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_INSTALL_TECHNOLOGY_MISMATCH);
            }

            AICLI_TERMINATE_CONTEXT(isUpgrade ? APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE : APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);
        }
    }

    void EnsureUpdateVersionApplicable(Execution::Context& context)
    {
        auto installedPackage = context.Get<Execution::Data::InstalledPackageVersion>();
        Utility::Version installedVersion = Utility::Version(installedPackage->GetProperty(PackageVersionProperty::Version));
        Utility::Version updateVersion(context.Get<Execution::Data::Manifest>().Version);

        if (!IsUpdateVersionAvailable(installedVersion, updateVersion))
        {
            context.Reporter.Info() << Resource::String::UpdateNoPackagesFound << std::endl
                << Resource::String::UpdateNoPackagesFoundReason << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
        }
    }

    void UpdateAllApplicable(Execution::Context& context)
    {
        const auto& matches = context.Get<Execution::Data::SearchResult>().Matches;
        std::vector<std::unique_ptr<Execution::Context>> packageSubContexts;
        bool updateAllFoundUpdate = false;
        int packagesWithUnknownVersionSkipped = 0;
        int packagesThatRequireExplicitSkipped = 0;
        int packagesSkippedInstallTechnologyMismatch = 0;
        int packagesSkippedByUpgradeDelay = 0;
        const auto upgradeDelay = Settings::User().Get<Settings::Setting::UpgradeDelayInDays>();
        const bool ignoreUpgradeDelay = context.Args.Contains(Execution::Args::Type::Force);

        for (const auto& match : matches)
        {
            // We want to do best effort to update all applicable updates regardless on previous update failure
            auto updateContextPtr = context.CreateSubContext();
            Execution::Context& updateContext = *updateContextPtr;
            auto previousThreadGlobals = updateContext.SetForCurrentThread();
            auto installedVersion = GetInstalledVersion(match.Package);

            updateContext.Add<Execution::Data::Package>(match.Package);

            // Filter out packages with unknown installed versions
            if (Utility::Version(installedVersion->GetProperty(PackageVersionProperty::Version)).IsUnknown() &&
                !context.Args.Contains(Execution::Args::Type::IncludeUnknown))
            {
                // we don't know what the package's version is and the user didn't ask to upgrade it anyway.
                AICLI_LOG(CLI, Info, << "Skipping " << match.Package->GetProperty(PackageProperty::Id) << " as it has unknown installed version");
                ++packagesWithUnknownVersionSkipped;
                continue;
            }

            updateContext <<
                Workflow::GetInstalledPackageVersion <<
                Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
                SelectLatestApplicableVersion(false);

            if (updateContext.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_INSTALL_TECHNOLOGY_MISMATCH)
            {
                AICLI_LOG(CLI, Info, << "Skipping " << match.Package->GetProperty(PackageProperty::Id)
                    << " as available upgrades use a different install technology");
                ++packagesSkippedInstallTechnologyMismatch;
                continue;
            }

            if (updateContext.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE)
            {
                continue;
            }

            if (!ignoreUpgradeDelay && upgradeDelay > std::chrono::hours{ 0 })
            {
                std::string_view releaseDate;
                if (updateContext.Contains(Execution::Data::Installer))
                {
                    const auto& installer = updateContext.Get<Execution::Data::Installer>();
                    if (installer.has_value() && !installer->ReleaseDate.empty())
                    {
                        releaseDate = installer->ReleaseDate;
                    }
                }

                if (releaseDate.empty() && updateContext.Contains(Execution::Data::Manifest))
                {
                    const auto& manifest = updateContext.Get<Execution::Data::Manifest>();
                    if (!manifest.DefaultInstallerInfo.ReleaseDate.empty())
                    {
                        releaseDate = manifest.DefaultInstallerInfo.ReleaseDate;
                    }
                }

                if (!IsUpgradeEligibleByDelay(releaseDate, upgradeDelay))
                {
                    AICLI_LOG(CLI, Info, << "Skipping " << match.Package->GetProperty(PackageProperty::Id)
                        << " as its selected upgrade does not meet the upgrade delay requirement");
                    ++packagesSkippedByUpgradeDelay;
                    continue;
                }
            }

            // Filter out packages that require explicit upgrade.
            // User-defined pins are handled when selecting the version to use.
            auto installedMetadata = updateContext.Get<Execution::Data::InstalledPackageVersion>()->GetMetadata();
            auto pinnedState = ConvertToPinTypeEnum(installedMetadata[PackageVersionMetadata::PinnedState]);
            if (pinnedState == PinType::PinnedByManifest)
            {
                // Note that for packages pinned by the manifest
                // this does not consider whether the update to be installed has
                // RequireExplicitUpgrade. While this has the downside of not working with
                // packages installed from another source, it ensures consistency with the
                // list of available updates (there we don't have the selected installer)
                // and at most we will update each package like this once.
                AICLI_LOG(CLI, Info, << "Skipping " << match.Package->GetProperty(PackageProperty::Id) << " as it requires explicit upgrade");
                ++packagesThatRequireExplicitSkipped;
                continue;
            }

            updateAllFoundUpdate = true;

            AddToPackageSubContextsIfNotPresent(packageSubContexts, std::move(updateContextPtr));
        }

        if (updateAllFoundUpdate)
        {
            context.Add<Execution::Data::PackageSubContexts>(std::move(packageSubContexts));
            context.Reporter.Info() << std::endl;

            ProcessMultiplePackages::Flags flags = ProcessMultiplePackages::Flags::None;
            if (Settings::User().Get<Settings::Setting::InstallSkipDependencies>() || context.Args.Contains(Execution::Args::Type::SkipDependencies))
            {
                flags = ProcessMultiplePackages::Flags::IgnoreDependencies;
            }

            context <<
                ProcessMultiplePackages(
                    Resource::String::PackageRequiresDependencies,
                    APPINSTALLER_CLI_ERROR_UPDATE_ALL_HAS_FAILURE,
                    flags,
                    { APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE });
        }

        if (packagesWithUnknownVersionSkipped > 0)
        {
            AICLI_LOG(CLI, Info, << packagesWithUnknownVersionSkipped << " package(s) skipped due to unknown installed version");
            context.Reporter.Info() << Resource::String::UpgradeUnknownVersionCount(packagesWithUnknownVersionSkipped) << std::endl;
        }

        if (packagesThatRequireExplicitSkipped > 0)
        {
            AICLI_LOG(CLI, Info, << packagesThatRequireExplicitSkipped << " package(s) skipped due to requiring explicit upgrade");
            context.Reporter.Info() << Resource::String::UpgradeRequireExplicitCount(packagesThatRequireExplicitSkipped) << std::endl;
        }

        if (packagesSkippedInstallTechnologyMismatch > 0)
        {
            AICLI_LOG(CLI, Info, << packagesSkippedInstallTechnologyMismatch << " package(s) skipped due to install technology mismatch");
            context.Reporter.Info() << Resource::String::UpgradeInstallTechnologyMismatchCount(packagesSkippedInstallTechnologyMismatch) << std::endl;
        }

        if (packagesSkippedByUpgradeDelay > 0)
        {
            AICLI_LOG(CLI, Info, << packagesSkippedByUpgradeDelay << " package(s) skipped due to upgrade delay setting");
            context.Reporter.Info() << Resource::String::UpgradeDelaySkippedCount(packagesSkippedByUpgradeDelay) << std::endl;
        }
    }

    void SelectSinglePackageVersionForInstallOrUpgrade::operator()(Execution::Context& context) const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_operationType != OperationType::Install && m_operationType != OperationType::Upgrade);

        context <<
            HandleSearchResultFailures <<
            EnsureOneMatchFromSearchResult(m_operationType) <<
            GetInstalledPackageVersion;

        if ( m_operationType != OperationType::Upgrade && 
            context.Contains(Execution::Data::InstalledPackageVersion) &&
            context.Get<Execution::Data::InstalledPackageVersion>() != nullptr )
        {
            if (context.Args.Contains(Execution::Args::Type::NoUpgrade))
            {
                AICLI_LOG(CLI, Warning, << "Found installed package, exiting installation.");
                context.Reporter.Warn() << Resource::String::PackageAlreadyInstalled << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PACKAGE_ALREADY_INSTALLED);
            }
            else
            {
                AICLI_LOG(CLI, Info, << "Found installed package, converting to upgrade flow");
                context.Reporter.Info() << Execution::ConvertToUpgradeFlowEmphasis << Resource::String::ConvertInstallFlowToUpgrade << std::endl;
                context.SetFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);
                m_operationType = OperationType::Upgrade;
            }
        }

        if (context.Args.Contains(Execution::Args::Type::Version))
        {
            // If version specified, use the version and verify applicability
            context << GetManifestFromPackage(/* considerPins */ true);

            if (m_operationType == OperationType::Upgrade && !m_allowDowngrade)
            {
                context << EnsureUpdateVersionApplicable;
            }

            context << SelectInstaller;
        }
        else
        {
            // Iterate through available versions to find latest applicable version.
            // This step also populates Manifest and Installer in context data.
            context << SelectLatestApplicableVersion(true);
        }

        context << EnsureApplicableInstaller;
    }

    void InstallOrUpgradeSinglePackage::operator()(Execution::Context& context) const
    {
        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), m_operationType != OperationType::Install && m_operationType != OperationType::Upgrade);

        context <<
            SearchSourceForSingle <<
            SelectSinglePackageVersionForInstallOrUpgrade(m_operationType) <<
            InstallSinglePackage;
    }
}
