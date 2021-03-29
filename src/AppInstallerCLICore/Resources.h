// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/LocIndependent.h>
#include <winget/Resources.h>

#include <winrt/Windows.ApplicationModel.Resources.h>

#include <iostream>

namespace AppInstaller::CLI::Resource
{
    using AppInstaller::StringResource::StringId;

    // Resource string identifiers.
    // This list can mostly be generated by the following PowerShell:
    //  > [xml]$res = Get-Content <winget.resw file path>
    //  > $res.root.data.name | % { "WINGET_DEFINE_RESOURCE_STRINGID($_);" }
    //
    struct String
    {
        WINGET_DEFINE_RESOURCE_STRINGID(AdjoinedNotFlagError);
        WINGET_DEFINE_RESOURCE_STRINGID(AdjoinedNotFoundError);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableArguments);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableCommands);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableHeader);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableOptions);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableSubcommands);
        WINGET_DEFINE_RESOURCE_STRINGID(BothManifestAndSearchQueryProvided);
        WINGET_DEFINE_RESOURCE_STRINGID(ChannelArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Command);
        WINGET_DEFINE_RESOURCE_STRINGID(CommandArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CommandLineArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CommandRequiresAdmin);
        WINGET_DEFINE_RESOURCE_STRINGID(CompleteCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CompleteCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CountArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(DisabledByGroupPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(Done);
        WINGET_DEFINE_RESOURCE_STRINGID(ExactArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExperimentalArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExperimentalCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExperimentalCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportIncludeVersionsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExportSourceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExtraPositionalError);
        WINGET_DEFINE_RESOURCE_STRINGID(FeatureDisabledMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesDisabled);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesEnabled);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesFeature);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesLink);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesMessageDisabledByPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesProperty);
        WINGET_DEFINE_RESOURCE_STRINGID(FeaturesStatus);
        WINGET_DEFINE_RESOURCE_STRINGID(FileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(FlagContainAdjoinedError);
        WINGET_DEFINE_RESOURCE_STRINGID(GetManifestResultVersionNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(HashCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HashCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpForDetails);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpLinkPreamble);
        WINGET_DEFINE_RESOURCE_STRINGID(IdArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportFileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportFileHasInvalidSchema);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportIgnorePackageVersionsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportIgnoreUnavailableArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportInstallFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportPackageAlreadyInstalled);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportSearchFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(ImportSourceNotInstalled);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimer1);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimer2);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimerMSStore);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationRequiresHigherWindows);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstalledPackageNotAvailable);
        WINGET_DEFINE_RESOURCE_STRINGID(InstalledPackageVersionNotAvailable);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerBlockedByPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerFailedSecurityCheck);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerFailedVirusScan);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchAdminBlock);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchError);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchOverridden);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashMismatchOverrideRequired);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerHashVerified);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowInstallSuccess);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallFlowStartingPackageInstall);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallForceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallScopeDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InteractiveArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidAliasError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidArgumentSpecifierError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidArgumentValueError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidJsonFile);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidNameError);
        WINGET_DEFINE_RESOURCE_STRINGID(LanguageArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(LicenseAgreement);
        WINGET_DEFINE_RESOURCE_STRINGID(Links);
        WINGET_DEFINE_RESOURCE_STRINGID(ListCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ListCommandShortDescription);
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
        WINGET_DEFINE_RESOURCE_STRINGID(MonikerArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(MsixArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(MsixSignatureHashFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreAppBlocked);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallGetEntitlementNetworkError);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallGetEntitlementNoStoreAccount);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallGetEntitlementServerError);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallGetEntitlementSuccess);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallOrUpdateFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreInstallTryGetEntitlement);
        WINGET_DEFINE_RESOURCE_STRINGID(MSStoreStoreClientBlocked);
        WINGET_DEFINE_RESOURCE_STRINGID(MultipleInstalledPackagesFound);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiplePackagesFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NameArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(NoApplicableInstallers);
        WINGET_DEFINE_RESOURCE_STRINGID(NoExperimentalFeaturesMessage);
        WINGET_DEFINE_RESOURCE_STRINGID(NoInstalledPackageFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NoPackageFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NoPackagesFoundInImportFile);
        WINGET_DEFINE_RESOURCE_STRINGID(NoUninstallInfoFound);
        WINGET_DEFINE_RESOURCE_STRINGID(NoVTArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(OpenSourceFailedNoMatch);
        WINGET_DEFINE_RESOURCE_STRINGID(OpenSourceFailedNoMatchHelp);
        WINGET_DEFINE_RESOURCE_STRINGID(OpenSourceFailedNoSourceDefined);
        WINGET_DEFINE_RESOURCE_STRINGID(Options);
        WINGET_DEFINE_RESOURCE_STRINGID(OutputFileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(OverrideArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Package);
        WINGET_DEFINE_RESOURCE_STRINGID(PendingWorkError);
        WINGET_DEFINE_RESOURCE_STRINGID(PoliciesDisabled);
        WINGET_DEFINE_RESOURCE_STRINGID(PoliciesEnabled);
        WINGET_DEFINE_RESOURCE_STRINGID(PoliciesPolicy);
        WINGET_DEFINE_RESOURCE_STRINGID(PoliciesState);
        WINGET_DEFINE_RESOURCE_STRINGID(PositionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(PreviewVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(PrivacyStatement);
        WINGET_DEFINE_RESOURCE_STRINGID(QueryArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(RainbowArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ReportIdentityFound);
        WINGET_DEFINE_RESOURCE_STRINGID(RequiredArgError);
        WINGET_DEFINE_RESOURCE_STRINGID(RetroArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchId);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchMatch);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchName);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchSource);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchTruncated);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingLoadFailure);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarnings);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningField);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsWarningValue);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowChannel);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(SilentArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SingleCharAfterDashError);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddAlreadyExistsDifferentArg);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddAlreadyExistsDifferentName);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddAlreadyExistsMatch);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddBegin);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceArgArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListArg);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListData);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListField);
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
        WINGET_DEFINE_RESOURCE_STRINGID(TagArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ThankYou);
        WINGET_DEFINE_RESOURCE_STRINGID(ThirdPartSoftwareNotices);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolInfoArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolVersionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(TooManyArgError);
        WINGET_DEFINE_RESOURCE_STRINGID(TooManyBehaviorsError);
        WINGET_DEFINE_RESOURCE_STRINGID(UnexpectedErrorExecutingCommand);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallAbandoned);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallFailedWithCode);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallFlowStartingPackageUninstall);
        WINGET_DEFINE_RESOURCE_STRINGID(UninstallFlowUninstallSuccess);
        WINGET_DEFINE_RESOURCE_STRINGID(UnrecognizedCommand);
        WINGET_DEFINE_RESOURCE_STRINGID(UpdateAllArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UpdateNotApplicable);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(UpgradeCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Usage);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateManifestArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VerboseLogsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyFileFailedIsDirectory);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyFileFailedNotExist);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyFileSignedMsix);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyPathFailedNotExist);
        WINGET_DEFINE_RESOURCE_STRINGID(VersionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VersionsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(WordArgumentDescription);
    };

    // A localized string
    struct LocString : public Utility::LocIndString
    {
        LocString() = default;

        LocString(StringId id);

        LocString(const LocString&) = default;
        LocString& operator=(const LocString&) = default;

        LocString(LocString&&) = default;
        LocString& operator=(LocString&&) = default;
    };

    // Utility class to load resources
    class Loader
    {
    public:
        // Gets the singleton instance of the resource loader.
        static const Loader& Instance();

        // Gets the the string resource value.
        std::string ResolveString(std::wstring_view resKey) const;

    private:
        winrt::Windows::ApplicationModel::Resources::ResourceLoader m_wingetLoader;

        Loader();
    };
}

inline std::ostream& operator<<(std::ostream& out, AppInstaller::CLI::Resource::StringId si)
{
    return (out << AppInstaller::CLI::Resource::LocString{ si });
}
