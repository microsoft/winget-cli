// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.Resources.h>

#include <iostream>
#include <string>


namespace AppInstaller::CLI::Resource
{
    using namespace std::string_view_literals;

#define WINGET_WIDE_STRINGIFY_HELP(_id_) L ## _id_
#define WINGET_WIDE_STRINGIFY(_id_) WINGET_WIDE_STRINGIFY_HELP(_id_)
#define WINGET_DEFINE_RESOURCE_STRINGID(_id_) static constexpr StringId _id_ { WINGET_WIDE_STRINGIFY(#_id_) ## sv }

    // A resource identifier
    struct StringId : public std::wstring_view
    {
        explicit constexpr StringId(std::wstring_view id) : std::wstring_view(id) {}
    };

    // Resource string identifiers.
    // This list can easily be generated by the following PowerShell:
    //  > [xml]$res = Get-Content <winget.resw file path>
    //  > $res.root.data.name | % { "WINGET_DEFINE_RESOURCE_STRINGID($_);" }
    struct String
    {
        WINGET_DEFINE_RESOURCE_STRINGID(AdjoinedNotFlagError);
        WINGET_DEFINE_RESOURCE_STRINGID(AdjoinedNotFoundError);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableArguments);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableCommands);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableOptions);
        WINGET_DEFINE_RESOURCE_STRINGID(AvailableSubcommands);
        WINGET_DEFINE_RESOURCE_STRINGID(ChannelArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Command);
        WINGET_DEFINE_RESOURCE_STRINGID(CommandArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(CountArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExactArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ExtraPositionalError);
        WINGET_DEFINE_RESOURCE_STRINGID(FileArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(FlagContainAdjoinedError);
        WINGET_DEFINE_RESOURCE_STRINGID(ForceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HashCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HashCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpForDetails);
        WINGET_DEFINE_RESOURCE_STRINGID(HelpLinkPreamble);
        WINGET_DEFINE_RESOURCE_STRINGID(IdArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimer1);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationDisclaimer2);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallationRequiresHigherWindows);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InteractiveArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidAliasError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidArgumentSpecifierError);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidNameError);
        WINGET_DEFINE_RESOURCE_STRINGID(LanguageArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(LicenseAgreement);
        WINGET_DEFINE_RESOURCE_STRINGID(Links);
        WINGET_DEFINE_RESOURCE_STRINGID(LocationArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(LogArgumentDescription);
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
        WINGET_DEFINE_RESOURCE_STRINGID(NameArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(NoApplicableInstallers);
        WINGET_DEFINE_RESOURCE_STRINGID(NoVTArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Options);
        WINGET_DEFINE_RESOURCE_STRINGID(OverrideArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(Package);
        WINGET_DEFINE_RESOURCE_STRINGID(PendingWorkError);
        WINGET_DEFINE_RESOURCE_STRINGID(PreviewVersion);
        WINGET_DEFINE_RESOURCE_STRINGID(PrivacyStatement);
        WINGET_DEFINE_RESOURCE_STRINGID(QueryArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(RainbowArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(RequiredArgError);
        WINGET_DEFINE_RESOURCE_STRINGID(RetroArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SearchCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SettingsCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ShowCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SilentArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SingleCharAfterDashError);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceAddCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceArgArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceListCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceNameArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceRemoveCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceRemoveCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceResetCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceTypeArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceUpdateCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(SourceUpdateCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(TagArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ThirdPartSoftwareNotices);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolInfoArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ToolVersionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(TooManyArgError);
        WINGET_DEFINE_RESOURCE_STRINGID(TooManyBehaviorsError);
        WINGET_DEFINE_RESOURCE_STRINGID(UnexpectedErrorExecutingCommand);
        WINGET_DEFINE_RESOURCE_STRINGID(UnrecognizedCommand);
        WINGET_DEFINE_RESOURCE_STRINGID(Usage);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateCommandLongDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateCommandShortDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(ValidateManifestArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VerboseLogsArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VerifyFileSignedMsix);
        WINGET_DEFINE_RESOURCE_STRINGID(VersionArgumentDescription);
        WINGET_DEFINE_RESOURCE_STRINGID(VersionsArgumentDescription);
    };

    // A localized string
    struct LocString
    {
        LocString() = default;

        LocString(StringId id);

        LocString(const LocString&) = default;
        LocString& operator=(const LocString&) = default;

        LocString(LocString&&) = default;
        LocString& operator=(LocString&&) = default;

        const std::string& get() const { return m_value; }

    private:
        std::string m_value;
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

inline std::ostream& operator<<(std::ostream& out, const AppInstaller::CLI::Resource::LocString& ls)
{
    return (out << ls.get());
}

inline std::ostream& operator<<(std::ostream& out, AppInstaller::CLI::Resource::StringId si)
{
    return (out << AppInstaller::CLI::Resource::LocString{ si });
}
