// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/Resources.h>

#include <winrt/Windows.ApplicationModel.Resources.h>

#include <iostream>

namespace AppInstaller::CLI::Resource
{
    using AppInstaller::StringResource::StringId;
    using AppInstaller::Resource::LocString;

    // Resource string identifiers.
    // This list can mostly be generated by the following PowerShell:
    //  > [xml]$res = Get-Content <winget.resw file path>
    //  > $res.root.data.name | % { "WINGET_DEFINE_RESOURCE_STRINGID($_);" }
    //
    struct String
    {
        WINGET_DEFINE_RESOURCE_STRINGID(AcceptPackageAgreementsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(AcceptSourceAgreementsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(AdjoinedNotFlagError);
        WINGET_DEFINE_RESOURCE_STRINGID(AdjoinedNotFoundError);
        WINGET_DEFINE_RESOURCE_STRINGID(AdminSettingDisabled);
        WINGET_DEFINE_RESOURCE_STRINGID(AdminSettingDisableDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(AdminSettingEnabled);
        WINGET_DEFINE_RESOURCE_STRINGID(AdminSettingEnableDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(AdminSettingHeader);
        WINGET_DEFINE_RESOURCE_STRINGID(ArchiveFailedMalwareScan);
        WINGET_DEFINE_RESOURCE_STRINGID(ArchiveFailedMalwareScanOverridden);
        WINGET_DEFINE_RESOURCE_STRINGID(ArgumentForSinglePackageProvidedWithMultipleQueries);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableArguments);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableCommandAliases);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableCommands);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableHeader);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableOptions);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableSubcommands);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableUpgrades);
        WINGET_DEFINE_RESOURCE_STRINGID(BothManifestAndSearchQueryProvided);
        WINGET_DEFINE_RESOURCE_STRINGID(Cancelled);
        WINGET_DEFINE_RESOURCE_STRINGID(CancellingOperation);
        WINGET_DEFINE_RESOURCE_STRINGID(ChannelArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Command);
        WINGET_DEFINE_RESOURCE_STRINGID(CommandArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CommandLineArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CommandRequiresAdmin);
        WINGET_DEFINE_RESOURCE_STRINGID(CompleteCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CompleteCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationAcceptWarningArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationApply);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationAssert);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationDescriptionWasTruncated);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationDisableMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationDisabledMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationDisablingMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationEnableArgumentError);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationEnableMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationEnabledMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationEnablingMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFailedToApply);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFailedToGetDetails);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFieldInvalidType);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFieldInvalidValue);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFieldMissing);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFileEmpty);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFileInvalid);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationFileVersionUnknown);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationGettingDetails);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationInform);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationInitializing);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationLocal);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationModuleNameOnly);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationModuleWithDetails);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationNotEnabledMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationReadingConfigFile);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationSettings);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationSuccessfullyApplied);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitAssertHadNegativeResult);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedConfigSet);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedDuringGet);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedDuringSet);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedDuringTest);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedInternal);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedPrecondition);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedSystemState);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitFailedUnitProcessing);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitHasDuplicateIdentifier);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitHasMissingDependency);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitManuallySkipped);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitModuleConflict);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitModuleImportFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitMultipleMatches);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitNotFoundInModule);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitNotRunDueToDependency);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitNotRunDueToFailedAssert);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitReturnedInvalidResult);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationUnitSkipped);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationWaitingOnAnother);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationWarning);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigurationWarningPrompt);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureShowCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureShowCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureTestCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureTestCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureValidateCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConfigureValidateCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ConvertInstallFlowToUpgrade);
        WINGET_DEFINE_RESOURCE_STRINGID(CountArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CountOutOfBoundsError);
        WINGET_DEFINE_RESOURCE_STRINGID(CustomSwitchesArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowInstall);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowSourceNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowSourceTooManyMatches);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowPackageVersionNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowNoInstallerFound);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowNoMinVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowNoSuitableInstallerFound);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowNoMatches);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesFlowContainsLoop);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesSkippedMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesManagementError);
        WINGET_DEFINE_RESOURCE_STRINGID(DependenciesManagementExitMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(DisabledByGroupPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(DisableAdminSettingFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(DisableInteractivityArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Done);
        WINGET_DEFINE_RESOURCE_STRINGID(Downloading);
        WINGET_DEFINE_RESOURCE_STRINGID(EnableAdminSettingFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(EnablingWindowsFeature);
        WINGET_DEFINE_RESOURCE_STRINGID(ExactArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExperimentalArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExperimentalCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExperimentalCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportedPackageRequiresLicenseAgreement);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportIncludeVersionsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportSourceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExternalDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ExtractArchiveFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(ExtractArchiveSucceeded);
        WINGET_DEFINE_RESOURCE_STRINGID(ExtractingArchive);
        WINGET_DEFINE_RESOURCE_STRINGID(ExtraPositionalError);
        WINGET_DEFINE_RESOURCE_STRINGID(FailedToEnableWindowsFeature);
        WINGET_DEFINE_RESOURCE_STRINGID(FailedToEnableWindowsFeatureOverridden);
        WINGET_DEFINE_RESOURCE_STRINGID(FailedToEnableWindowsFeatureOverrideRequired);
        WINGET_DEFINE_RESOURCE_STRINGID(FailedToRefreshPathWarning);
        WINGET_DEFINE_RESOURCE_STRINGID(FeatureDisabledByAdminSettingMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(FeatureDisabledMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesDisabled);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesEnabled);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesFeature);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesLink);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesMessageDisabledByBuild);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesMessageDisabledByPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesProperty);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesStatus);
        WINGET_DEFINE_RESOURCE_STRINGID(FileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(FileNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(FilesRemainInInstallDirectory);
        WINGET_DEFINE_RESOURCE_STRINGID(FlagContainAdjoinedError);
        WINGET_DEFINE_RESOURCE_STRINGID(ForceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(GatedVersionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(GetManifestResultVersionNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(HashCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HashCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HashOverrideArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HeaderArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HeaderArgumentNotApplicableForNonRestSourceWarning);
        WINGET_DEFINE_RESOURCE_STRINGID(HeaderArgumentNotApplicableWithoutSource);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpForDetails);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpLinkPreamble);
        WINGET_DEFINE_RESOURCE_STRINGID(IdArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(IgnoreLocalArchiveMalwareScanArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportCommandReportDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportFileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportFileHasInvalidSchema);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportIgnorePackageVersionsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportIgnoreUnavailableArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportInstallFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportSourceNotInstalled);
        WINGET_DEFINE_RESOURCE_STRINGID(IncludePinnedArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(IncludeUnknownArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(IncompatibleArgumentsProvided);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallAndUpgradeCommandsReportDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallArchitectureArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationAbandoned);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimer1);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimer2);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimerMSStore);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstalledPackageNotAvailable);
        WINGET_DEFINE_RESOURCE_STRINGID(InstalledPackageVersionNotAvailable);
        WINGET_DEFINE_RESOURCE_STRINGID(InstalledScopeArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerAbortsTerminal);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerElevationExpected);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerBlockedByPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerFailedSecurityCheck);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerFailedVirusScan);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerFailedWithCode);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchAdminBlock);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchError);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchOverridden);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchOverrideRequired);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashVerified);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerLogAvailable);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerProhibitsElevation);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerRequiresInstallLocation);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallersAbortTerminal);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallersRequireInstallLocation);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowInstallSuccess);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowRegistrationDeferred);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeAlreadyInstalled);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeBlockedByPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeCancelledByUser);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeContactSupport);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeDiskFull);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeDowngrade);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeFileInUse);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeInstallInProgress);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeInsufficientMemory);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeInvalidParameter);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeMissingDependency);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeNoNetwork);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodePackageInUse);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodePackageInUseByApplication);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeRebootInitiated);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeRebootRequiredForInstall);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeRebootRequiredToFinish);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowReturnCodeSystemNotSupported);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowStartingPackageInstall);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFullPackageDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallLocationNotProvided);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallScopeDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallStubPackageDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallWaitingOnAnother);
        WINGET_DEFINE_RESOURCE_STRINGID(InteractiveArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidAliasError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidArgumentSpecifierError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidArgumentValueError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidArgumentValueErrorWithoutValidValues);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidArgumentWithoutQueryError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidJsonFile);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidNameError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidPathToNestedInstaller);
        WINGET_DEFINE_RESOURCE_STRINGID(KeyDirectoriesHeader);
        WINGET_DEFINE_RESOURCE_STRINGID(LicenseAgreement);
        WINGET_DEFINE_RESOURCE_STRINGID(Links);
        WINGET_DEFINE_RESOURCE_STRINGID(ListCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ListCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(LocaleArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(LocationArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(LogArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Logs);
        WINGET_DEFINE_RESOURCE_STRINGID(MainCopyrightNotice);
        WINGET_DEFINE_RESOURCE_STRINGID(MainHomepage);
        WINGET_DEFINE_RESOURCE_STRINGID(ManifestArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ManifestValidationFail);
        WINGET_DEFINE_RESOURCE_STRINGID(ManifestValidationSuccess);
        WINGET_DEFINE_RESOURCE_STRINGID(ManifestValidationWarning);
        WINGET_DEFINE_RESOURCE_STRINGID(MissingArgumentError);
        WINGET_DEFINE_RESOURCE_STRINGID(ModifiedPathRequiresShellRestart);
        WINGET_DEFINE_RESOURCE_STRINGID(MonikerArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(MsixArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(MsixSignatureHashFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreAppBlocked);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallOrUpdateFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallTryGetEntitlement);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreStoreClientBlocked);
        WINGET_DEFINE_RESOURCE_STRINGID(MultipleExclusiveArgumentsProvided);
        WINGET_DEFINE_RESOURCE_STRINGID(MultipleInstalledPackagesFound);
        WINGET_DEFINE_RESOURCE_STRINGID(MultipleNonPortableNestedInstallersSpecified);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiplePackagesFound);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiQueryArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiQueryPackageAlreadyInstalled);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiQueryPackageNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiQuerySearchFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiQuerySearchFoundMultiple);
        WINGET_DEFINE_RESOURCE_STRINGID(NameArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(NestedInstallerNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NestedInstallerNotSpecified);
        WINGET_DEFINE_RESOURCE_STRINGID(NestedInstallerNotSupported);
        WINGET_DEFINE_RESOURCE_STRINGID(NoApplicableInstallers);
        WINGET_DEFINE_RESOURCE_STRINGID(NoExperimentalFeaturesMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(NoInstalledPackageFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NoPackageFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NoPackageSelectionArgumentProvided);
        WINGET_DEFINE_RESOURCE_STRINGID(NoPackagesFoundInImportFile);
        WINGET_DEFINE_RESOURCE_STRINGID(Notes);
        WINGET_DEFINE_RESOURCE_STRINGID(NoUninstallInfoFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NoUpgradeArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(NoVTArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(OpenLogsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(OpenSourceFailedNoMatch);
        WINGET_DEFINE_RESOURCE_STRINGID(OpenSourceFailedNoMatchHelp);
        WINGET_DEFINE_RESOURCE_STRINGID(OpenSourceFailedNoSourceDefined);
        WINGET_DEFINE_RESOURCE_STRINGID(Options);
        WINGET_DEFINE_RESOURCE_STRINGID(OutputFileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(OverrideArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(OverwritingExistingFileAtMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(Package);
        WINGET_DEFINE_RESOURCE_STRINGID(PackageAgreementsNotAgreedTo);
        WINGET_DEFINE_RESOURCE_STRINGID(PackageAgreementsPrompt);
        WINGET_DEFINE_RESOURCE_STRINGID(PackageAlreadyInstalled);
        WINGET_DEFINE_RESOURCE_STRINGID(PackageDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(PackageIsPinned);
        WINGET_DEFINE_RESOURCE_STRINGID(PendingWorkError);
        WINGET_DEFINE_RESOURCE_STRINGID(PinAddBlockingArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinAddCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinAddCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinAdded);
        WINGET_DEFINE_RESOURCE_STRINGID(PinAlreadyExists);
        WINGET_DEFINE_RESOURCE_STRINGID(PinCannotOpenIndex);
        WINGET_DEFINE_RESOURCE_STRINGID(PinCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinDoesNotExist);
        WINGET_DEFINE_RESOURCE_STRINGID(PinExistsOverwriting);
        WINGET_DEFINE_RESOURCE_STRINGID(PinExistsUseForceArg);
        WINGET_DEFINE_RESOURCE_STRINGID(PinInstalledArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinInstalledSource);
        WINGET_DEFINE_RESOURCE_STRINGID(PinListCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinListCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinNoPinsExist);
        WINGET_DEFINE_RESOURCE_STRINGID(PinRemoveCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinRemoveCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinRemovedSuccessfully);
        WINGET_DEFINE_RESOURCE_STRINGID(PinResetCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinResetCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PinResetSuccessful);
        WINGET_DEFINE_RESOURCE_STRINGID(PinResettingAll);
        WINGET_DEFINE_RESOURCE_STRINGID(PinResetUseForceArg);
        WINGET_DEFINE_RESOURCE_STRINGID(PinType);
        WINGET_DEFINE_RESOURCE_STRINGID(PinVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(PoliciesPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableHashMismatchOverridden);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableHashMismatchOverrideRequired);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableAliasAdded);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableInstallFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableLinksMachine);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableLinksUser);
        WINGET_DEFINE_RESOURCE_STRINGID(PortablePackageAlreadyExists);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableRegistryCollisionOverridden);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableRoot);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableRoot86);
        WINGET_DEFINE_RESOURCE_STRINGID(PortableRootUser);
        WINGET_DEFINE_RESOURCE_STRINGID(PositionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PreserveArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PressEnterToContinue);
        WINGET_DEFINE_RESOURCE_STRINGID(PrivacyStatement);
        WINGET_DEFINE_RESOURCE_STRINGID(ProductCodeArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PromptForInstallRoot);
        WINGET_DEFINE_RESOURCE_STRINGID(PromptOptionNo);
        WINGET_DEFINE_RESOURCE_STRINGID(PromptOptionYes);
        WINGET_DEFINE_RESOURCE_STRINGID(PromptToProceed);
        WINGET_DEFINE_RESOURCE_STRINGID(PurgeArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PurgeInstallDirectory);
        WINGET_DEFINE_RESOURCE_STRINGID(QueryArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(RainbowArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(RebootRequiredToEnableWindowsFeatureOverridden);
        WINGET_DEFINE_RESOURCE_STRINGID(RebootRequiredToEnableWindowsFeatureOverrideRequired);
        WINGET_DEFINE_RESOURCE_STRINGID(RelatedLink);
        WINGET_DEFINE_RESOURCE_STRINGID(RenameArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ReparsePointsNotSupportedError);
        WINGET_DEFINE_RESOURCE_STRINGID(ReportIdentityForAgreements);
        WINGET_DEFINE_RESOURCE_STRINGID(ReportIdentityFound);
        WINGET_DEFINE_RESOURCE_STRINGID(RequiredArgError);
        WINGET_DEFINE_RESOURCE_STRINGID(ReservedFilenameError);
        WINGET_DEFINE_RESOURCE_STRINGID(RetroArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchFailureError);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchFailureErrorListMatches);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchFailureErrorNoMatches);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchFailureWarning);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchId);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchMatch);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchName);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchSource);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchTruncated);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(SeeLineAndColumn);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingLoadFailure);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsExportCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsExportCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningField);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarnings);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningValue);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowChannel);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelAgreements);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelAuthor);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelCopyright);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelCopyrightUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelDocumentation);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelExternalDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstallationNotes);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstaller);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstallerLocale);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstallerProductId);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstallerReleaseDate);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstallerSha256);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstallerType);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelInstallerUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelLicense);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelLicenseUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelMoniker);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelPackageDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelPackageUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelPrivacyUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelPublisher);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelPublisherSupportUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelPublisherUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelPurchaseUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelReleaseNotes);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelReleaseNotesUrl);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelTags);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelWindowsFeaturesDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowLabelWindowsLibrariesDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(SilentArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SingleCharAfterDashError);
        WINGET_DEFINE_RESOURCE_STRINGID(SkipDependenciesArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddAlreadyExistsDifferentArg);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddAlreadyExistsDifferentName);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddAlreadyExistsMatch);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddBegin);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddOpenSourceFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAgreementsMarketMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAgreementsNotAgreedTo);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAgreementsPrompt);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAgreementsTitle);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceArgArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(DependencySourceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceExportCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceExportCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListAdditionalSource);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListAllowedSource);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListArg);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListData);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListField);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListIdentifier);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListName);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListNoneFound);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListNoSources);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListType);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListUpdated);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListUpdatedNever);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListValue);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceNameArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceOpenFailedSuggestion);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceOpenPredefinedFailedSuggestion);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceOpenWithFailedUpdate);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceRemoveAll);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceRemoveCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceRemoveCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceRemoveOne);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetAll);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetForceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetListAndOverridePreamble);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetOne);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceTypeArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceUpdateAll);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceUpdateCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceUpdateCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceUpdateOne);
        WINGET_DEFINE_RESOURCE_STRINGID(StateDisabled);
        WINGET_DEFINE_RESOURCE_STRINGID(StateEnabled);
        WINGET_DEFINE_RESOURCE_STRINGID(StateHeader);
        WINGET_DEFINE_RESOURCE_STRINGID(SystemArchitecture);
        WINGET_DEFINE_RESOURCE_STRINGID(TagArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ThankYou);
        WINGET_DEFINE_RESOURCE_STRINGID(ThirdPartSoftwareNotices);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolInfoArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolVersionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(TooManyArgError);
        WINGET_DEFINE_RESOURCE_STRINGID(TooManyBehaviorsError);
        WINGET_DEFINE_RESOURCE_STRINGID(UnableToPurgeInstallDirectory);
        WINGET_DEFINE_RESOURCE_STRINGID(UnexpectedErrorExecutingCommand);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallAbandoned);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallCommandReportDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallFailedWithCode);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallFlowStartingPackageUninstall);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallFlowUninstallSuccess);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallPreviousArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UnrecognizedCommand);
        WINGET_DEFINE_RESOURCE_STRINGID(UnsupportedArgument);
        WINGET_DEFINE_RESOURCE_STRINGID(UpdateAllArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UpdateNotApplicable);
        WINGET_DEFINE_RESOURCE_STRINGID(UpdateNotApplicableReason);
        WINGET_DEFINE_RESOURCE_STRINGID(UpdateNoPackagesFound);
        WINGET_DEFINE_RESOURCE_STRINGID(UpdateNoPackagesFoundReason);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeAvailableForPinned);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeBlockedByPinCount);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeDifferentInstallTechnology);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeDifferentInstallTechnologyInNewerVersions);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeIsPinned);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradePinnedByUserCount);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeRequireExplicitCount);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeUnknownVersionCount);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeUnknownVersionExplanation);
        WINGET_DEFINE_RESOURCE_STRINGID(Usage);
        WINGET_DEFINE_RESOURCE_STRINGID(UserSettings);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateCommandReportDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateManifestArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VerboseLogsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyFileFailedIsDirectory);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyFileFailedNotExist);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyFileSignedMsix);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyPathFailedNotExist);
        WINGET_DEFINE_RESOURCE_STRINGID(VersionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VersionsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(WaitArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(WindowsFeatureNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(WindowsFeaturesDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(WindowsLibrariesDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(WindowsStoreTerms);
        WINGET_DEFINE_RESOURCE_STRINGID(WindowsPackageManager);
        WINGET_DEFINE_RESOURCE_STRINGID(WindowsPackageManagerPreview);
        WINGET_DEFINE_RESOURCE_STRINGID(WordArgumentDescription);
    };

    // Fixed strings are not localized, but we use a similar system to prevent duplication
    enum class FixedString
    {
        ProductName,
    };

    Utility::LocIndView GetFixedString(FixedString fs);
}

inline std::ostream& operator<<(std::ostream& out, AppInstaller::CLI::Resource::FixedString fs)
{
    return (out << GetFixedString(fs));
}
