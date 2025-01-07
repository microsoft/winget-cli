// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UpgradeCommand.h"
#include "Workflows/CompletionFlow.h"
#include "Workflows/DownloadFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/MultiQueryFlow.h"
#include "Workflows/UpdateFlow.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/DependenciesFlow.h"
#include "Resources.h"
#include <winget/LocIndependent.h>

namespace AppInstaller::CLI
{
    using namespace AppInstaller::CLI::Execution;
    using namespace AppInstaller::Manifest;
    using namespace AppInstaller::CLI::Workflow;
    using namespace AppInstaller::Utility::literals;

    namespace
    {
        // Determines whether we should list available upgrades, instead
        // of performing an upgrade
        bool ShouldListUpgrade(const Execution::Args& args, ArgTypeCategory argCategories = ArgTypeCategory::None)
        {
            if (argCategories == ArgTypeCategory::None)
            {
                argCategories = Argument::GetCategoriesPresent(args);
            }

            // Valid arguments for list are only those related to the sources and which packages to include (e.g. --include-unknown).
            // Instead of checking for them, we check that there aren't any other arguments present.
            return !args.Contains(Args::Type::All) &&
                WI_AreAllFlagsClear(argCategories, ArgTypeCategory::Manifest | ArgTypeCategory::PackageQuery | ArgTypeCategory::InstallerBehavior);
        }
    }

    std::vector<Argument> UpgradeCommand::GetArguments() const
    {
        return {
            Argument::ForType(Args::Type::MultiQuery),      // -q
            Argument::ForType(Args::Type::Manifest),        // -m
            Argument::ForType(Args::Type::Id),
            Argument::ForType(Args::Type::Name),
            Argument::ForType(Args::Type::Moniker),
            Argument::ForType(Args::Type::Version),         // -v
            Argument::ForType(Args::Type::Channel),
            Argument::ForType(Args::Type::Source),          // -s
            Argument::ForType(Args::Type::Exact),           // -e
            Argument::ForType(Args::Type::Interactive),     // -i
            Argument::ForType(Args::Type::Silent),          // -h
            Argument::ForType(Args::Type::Purge),
            Argument::ForType(Args::Type::Log),             // -o
            Argument::ForType(Args::Type::CustomSwitches),
            Argument::ForType(Args::Type::Override),
            Argument::ForType(Args::Type::InstallLocation), // -l
            Argument{ Args::Type::InstallScope, Resource::String::InstalledScopeArgumentDescription, ArgumentType::Standard, Argument::Visibility::Help },
            Argument::ForType(Args::Type::InstallArchitecture), // -a
            Argument::ForType(Args::Type::InstallerType),
            Argument::ForType(Args::Type::Locale),
            Argument::ForType(Args::Type::HashOverride),
            Argument::ForType(Args::Type::AllowReboot),
            Argument::ForType(Args::Type::SkipDependencies),
            Argument::ForType(Args::Type::IgnoreLocalArchiveMalwareScan),
            Argument::ForType(Args::Type::AcceptPackageAgreements),
            Argument::ForType(Args::Type::AcceptSourceAgreements),
            Argument::ForType(Args::Type::CustomHeader),
            Argument::ForType(Args::Type::AuthenticationMode),
            Argument::ForType(Args::Type::AuthenticationAccount),
            Argument{ Args::Type::All, Resource::String::UpdateAllArgumentDescription, ArgumentType::Flag },
            Argument{ Args::Type::IncludeUnknown, Resource::String::IncludeUnknownArgumentDescription, ArgumentType::Flag },
            Argument{ Args::Type::IncludePinned, Resource::String::IncludePinnedArgumentDescription, ArgumentType::Flag},
            Argument::ForType(Args::Type::UninstallPrevious),
            Argument::ForType(Args::Type::Force),
        };
    }

    Resource::LocString UpgradeCommand::ShortDescription() const
    {
        return { Resource::String::UpgradeCommandShortDescription };
    }

    Resource::LocString UpgradeCommand::LongDescription() const
    {
        return { Resource::String::UpgradeCommandLongDescription };
    }

    void UpgradeCommand::Complete(Execution::Context& context, Execution::Args::Type valueType) const
    {
        if (valueType == Execution::Args::Type::Manifest ||
            valueType == Execution::Args::Type::Log ||
            valueType == Execution::Args::Type::Override ||
            valueType == Execution::Args::Type::InstallLocation)
        {
            // Intentionally output nothing to allow pass through to filesystem.
            return;
        }

        context <<
            OpenSource() <<
            OpenCompositeSource(Repository::PredefinedSource::Installed);

        switch (valueType)
        {
        case Execution::Args::Type::MultiQuery:
            context <<
                RequireCompletionWordNonEmpty <<
                SearchSourceForManyCompletion <<
                CompleteWithMatchedField;
            break;
        case Execution::Args::Type::Id:
        case Execution::Args::Type::Name:
        case Execution::Args::Type::Moniker:
        case Execution::Args::Type::Version:
        case Execution::Args::Type::Channel:
        case Execution::Args::Type::Source:
            context <<
                CompleteWithSingleSemanticsForValueUsingExistingSource(valueType);
            break;
        case Args::Type::InstallArchitecture:
        case Args::Type::Locale:
            // May well move to CompleteWithSingleSemanticsForValue,
            // but for now output nothing.
            context <<
                Workflow::CompleteWithEmptySet;
            break;
        }
    }

    Utility::LocIndView UpgradeCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-upgrade"_liv;
    }

    void UpgradeCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        const auto argCategories = Argument::GetCategoriesAndValidateCommonArguments(execArgs, /* requirePackageSelectionArg */ false);

        if (!ShouldListUpgrade(execArgs, argCategories) &&
            WI_IsFlagClear(argCategories, ArgTypeCategory::PackageQuery) &&
            WI_IsFlagSet(argCategories, ArgTypeCategory::SingleInstallerBehavior))
        {
            throw CommandException(Resource::String::InvalidArgumentWithoutQueryError);
        }
    }

    void UpgradeCommand::ExecuteInternal(Execution::Context& context) const
    {
        context.SetFlags(Execution::ContextFlag::InstallerExecutionUseUpdate);

        // Only allow for source failures when doing a list of available upgrades.
        // We have to set it now to allow for source open failures to also just warn.
        if (ShouldListUpgrade(context.Args))
        {
            context.SetFlags(Execution::ContextFlag::TreatSourceFailuresAsWarning);
        }

        context <<
            Workflow::InitializeInstallerDownloadAuthenticatorsMap <<
            Workflow::ReportExecutionStage(ExecutionStage::Discovery) <<
            Workflow::OpenSource() <<
            Workflow::OpenCompositeSource(Workflow::DetermineInstalledSource(context));

        if (ShouldListUpgrade(context.Args))
        {
            // Upgrade with no args list packages with updates available
            context <<
                SearchSourceForMany <<
                HandleSearchResultFailures <<
                EnsureMatchesFromSearchResult(OperationType::Upgrade) <<
                ReportListResult(true);
        }
        else if (context.Args.Contains(Execution::Args::Type::All))
        {
            // --all switch updates all packages found
            context <<
                SearchSourceForMany <<
                HandleSearchResultFailures <<
                EnsureMatchesFromSearchResult(OperationType::Upgrade) <<
                ReportListResult(true) <<
                UpdateAllApplicable;
        }
        else if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            // --manifest case where new manifest is provided
            context <<
                GetManifestFromArg <<
                SearchSourceUsingManifest <<
                EnsureOneMatchFromSearchResult(OperationType::Upgrade) <<
                GetInstalledPackageVersion <<
                EnsureUpdateVersionApplicable <<
                SelectInstaller <<
                EnsureApplicableInstaller <<
                InstallSinglePackage;
        }
        else
        {
            // The remaining case: search for specific packages to update
            if (!context.Args.Contains(Execution::Args::Type::MultiQuery))
            {
                context << Workflow::InstallOrUpgradeSinglePackage(OperationType::Upgrade);
            }
            else
            {
                bool skipDependencies = Settings::User().Get<Settings::Setting::InstallSkipDependencies>() || context.Args.Contains(Execution::Args::Type::SkipDependencies);
                context <<
                    Workflow::GetMultiSearchRequests <<
                    Workflow::SearchSubContextsForSingle(OperationType::Upgrade) <<
                    Workflow::ReportExecutionStage(Workflow::ExecutionStage::Execution) <<
                    Workflow::ProcessMultiplePackages(
                        Resource::String::PackageRequiresDependencies,
                        APPINSTALLER_CLI_ERROR_MULTIPLE_INSTALL_FAILED,
                        {}, true, skipDependencies);
            }
        }
    }
}
