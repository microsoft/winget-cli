// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRepositorySource.h"

#include "CompositeSource.h"
#include "SourceFactory.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include "Rest/RestSourceFactory.h"

#include <winget/GroupPolicy.h>

namespace AppInstaller::Repository
{
    using namespace Settings;

    using namespace std::chrono_literals;
    using namespace std::string_view_literals;

    constexpr std::string_view s_SourcesYaml_Sources = "Sources"sv;
    constexpr std::string_view s_SourcesYaml_Source_Name = "Name"sv;
    constexpr std::string_view s_SourcesYaml_Source_Type = "Type"sv;
    constexpr std::string_view s_SourcesYaml_Source_Arg = "Arg"sv;
    constexpr std::string_view s_SourcesYaml_Source_Data = "Data"sv;
    constexpr std::string_view s_SourcesYaml_Source_Identifier = "Identifier"sv;
    constexpr std::string_view s_SourcesYaml_Source_IsTombstone = "IsTombstone"sv;

    constexpr std::string_view s_MetadataYaml_Sources = "Sources"sv;
    constexpr std::string_view s_MetadataYaml_Source_Name = "Name"sv;
    constexpr std::string_view s_MetadataYaml_Source_LastUpdate = "LastUpdate"sv;

    constexpr std::string_view s_Source_WingetCommunityDefault_Name = "winget"sv;
    constexpr std::string_view s_Source_WingetCommunityDefault_Arg = "https://winget.azureedge.net/cache"sv;
    constexpr std::string_view s_Source_WingetCommunityDefault_Data = "Microsoft.Winget.Source_8wekyb3d8bbwe"sv;
    constexpr std::string_view s_Source_WingetCommunityDefault_Identifier = "Microsoft.Winget.Source_8wekyb3d8bbwe"sv;

    constexpr std::string_view s_Source_WingetMSStoreDefault_Name = "msstore"sv;
    constexpr std::string_view s_Source_WingetMSStoreDefault_Arg = "https://winget.azureedge.net/msstore"sv;
    constexpr std::string_view s_Source_WingetMSStoreDefault_Data = "Microsoft.Winget.MSStore.Source_8wekyb3d8bbwe"sv;
    constexpr std::string_view s_Source_WingetMSStoreDefault_Identifier = "Microsoft.Winget.MSStore.Source_8wekyb3d8bbwe"sv;

    namespace
    {
        // SourceDetails with additional data used by this file.
        struct SourceDetailsInternal : public SourceDetails
        {
            // If true, this is a tombstone, marking the deletion of a source at a lower priority origin.
            bool IsTombstone = false;
        };

        // Checks whether a default source is enabled with the current settings.
        // onlyExplicit determines whether we consider the not-configured state to be enabled or not.
        bool IsDefaultSourceEnabled(std::string_view sourceToLog, ExperimentalFeature::Feature feature, bool onlyExplicit, TogglePolicy::Policy policy)
        {
            if (!ExperimentalFeature::IsEnabled(feature))
            {
                // No need to log here
                return false;
            }

            if (onlyExplicit)
            {
                // No need to log here
                return GroupPolicies().GetState(policy) == PolicyState::Enabled;
            }

            if (!GroupPolicies().IsEnabled(policy))
            {
                AICLI_LOG(Repo, Info, << "The default source " << sourceToLog << " is disabled due to Group Policy");
                return false;
            }

            return true;
        }

        bool IsWingetCommunityDefaultSourceEnabled(bool onlyExplicit = false)
        {
            return IsDefaultSourceEnabled(s_Source_WingetCommunityDefault_Name, ExperimentalFeature::Feature::None, onlyExplicit, TogglePolicy::Policy::DefaultSource);
        }

        bool IsWingetMSStoreDefaultSourceEnabled(bool onlyExplicit = false)
        {
            return IsDefaultSourceEnabled(s_Source_WingetMSStoreDefault_Name, ExperimentalFeature::Feature::ExperimentalMSStore, onlyExplicit, TogglePolicy::Policy::MSStoreSource);
        }

        template<ValuePolicy P>
        std::optional<SourceFromPolicy> FindSourceInPolicy(std::string_view name, std::string_view type, std::string_view arg)
        {
            auto sourcesOpt = GroupPolicies().GetValueRef<P>();
            if (!sourcesOpt.has_value())
            {
                return std::nullopt;
            }

            const auto& sources = sourcesOpt->get();
            auto source = std::find_if(
                sources.begin(),
                sources.end(),
                [&](const SourceFromPolicy& policySource)
                {
                    return Utility::ICUCaseInsensitiveEquals(name, policySource.Name) && Utility::ICUCaseInsensitiveEquals(type, policySource.Type) && arg == policySource.Arg;
                });

            if (source == sources.end())
            {
                return std::nullopt;
            }

            return *source;
        }

        template<ValuePolicy P>
        bool IsSourceInPolicy(std::string_view name, std::string_view type, std::string_view arg)
        {
            return FindSourceInPolicy<P>(name, type, arg).has_value();
        }

        // Checks whether the Group Policy allows this user source.
        // If it does it returns None, otherwise it returns which policy is blocking it.
        // Note that this applies to user sources that are being added as well as user sources
        // that already existed when the Group Policy came into effect.
        TogglePolicy::Policy GetPolicyBlockingUserSource(std::string_view name, std::string_view type, std::string_view arg, bool isTombstone)
        {
            // Reasons for not allowing:
            //  1. The source is a tombstone for default source that is explicitly enabled
            //  2. The source is a default source that is disabled
            //  3. The source has the same name as a default source that is explicitly enabled (to prevent shadowing)
            //  4. Allowed sources are disabled, blocking all user sources
            //  5. There is an explicit list of allowed sources and this source is not in it
            //
            // We don't need to check sources added by policy as those have higher priority.
            //
            // Use the name and arg to match sources as we don't have the identifier before adding.

            // Case 1:
            // The source is a tombstone and we need the policy to be explicitly enabled.
            if (isTombstone)
            {
                if (name == s_Source_WingetCommunityDefault_Name && IsWingetCommunityDefaultSourceEnabled(true))
                {
                    return TogglePolicy::Policy::DefaultSource;
                }

                if (name == s_Source_WingetMSStoreDefault_Name && IsWingetMSStoreDefaultSourceEnabled(true))
                {
                    return TogglePolicy::Policy::MSStoreSource;
                }

                // Any other tombstone is allowed
                return TogglePolicy::Policy::None;
            }

            // Case 2:
            //  - The source is not a tombstone and we don't need the policy to be explicitly enabled.
            //  - Check only against the source argument and type as the user source may have a different name.
            //  - Do a case insensitive check as the domain portion of the URL is case insensitive,
            //    and we don't need case sensitivity for the rest as we control the domain.
            if (Utility::CaseInsensitiveEquals(arg, s_Source_WingetCommunityDefault_Arg) &&
                Utility::CaseInsensitiveEquals(type, Microsoft::PreIndexedPackageSourceFactory::Type()))
            {
                return IsWingetCommunityDefaultSourceEnabled(false) ? TogglePolicy::Policy::None : TogglePolicy::Policy::DefaultSource;
            }

            if (Utility::CaseInsensitiveEquals(arg, s_Source_WingetMSStoreDefault_Arg) &&
                Utility::CaseInsensitiveEquals(type, Microsoft::PreIndexedPackageSourceFactory::Type()))
            {
                return IsWingetMSStoreDefaultSourceEnabled(false) ? TogglePolicy::Policy::None : TogglePolicy::Policy::MSStoreSource;
            }

            // Case 3:
            // If the source has the same name as a default source, it is shadowing with a different argument
            // (as it didn't match above). We only care if Group Policy requires the default source.
            if (name == s_Source_WingetCommunityDefault_Name && IsWingetCommunityDefaultSourceEnabled(true))
            {
                AICLI_LOG(Repo, Warning, << "User source is not allowed as it shadows the default source. Name [" << name << "]. Arg [" << arg << "] Type [" << type << ']');
                return TogglePolicy::Policy::DefaultSource;
            }

            if (name == s_Source_WingetMSStoreDefault_Name && IsWingetMSStoreDefaultSourceEnabled(true))
            {
                AICLI_LOG(Repo, Warning, << "User source is not allowed as it shadows the default MS Store source. Name [" << name << "]. Arg [" << arg << "] Type [" << type << ']');
                return TogglePolicy::Policy::MSStoreSource;
            }

            // Case 4:
            // The guard in the source add command should already block adding.
            // This check drops existing user sources.
            auto allowedSourcesPolicy = GroupPolicies().GetState(TogglePolicy::Policy::AllowedSources);
            if (allowedSourcesPolicy == PolicyState::Disabled)
            {
                AICLI_LOG(Repo, Warning, << "User sources are disabled by Group Policy");
                return TogglePolicy::Policy::AllowedSources;
            }

            // Case 5:
            if (allowedSourcesPolicy == PolicyState::Enabled)
            {
                if (!IsSourceInPolicy<ValuePolicy::AllowedSources>(name, type, arg))
                {
                    AICLI_LOG(Repo, Warning, << "Source is not in the Group Policy allowed list. Name [" << name << "]. Arg [" << arg << "] Type [" << type << ']');
                    return TogglePolicy::Policy::AllowedSources;
                }
            }

            return TogglePolicy::Policy::None;
        }

        bool IsUserSourceAllowedByPolicy(std::string_view name, std::string_view type, std::string_view arg, bool isTombstone)
        {
            return GetPolicyBlockingUserSource(name, type, arg, isTombstone) == TogglePolicy::Policy::None;
        }

        void EnsureSourceIsRemovable(const SourceDetailsInternal& source)
        {
            // Block removing sources added by Group Policy
            if (source.Origin == SourceOrigin::GroupPolicy)
            {
                AICLI_LOG(Repo, Error, << "Cannot remove source added by Group Policy");
                throw GroupPolicyException(TogglePolicy::Policy::AdditionalSources);
            }

            // Block removing default sources required by Group Policy.
            if (source.Origin == SourceOrigin::Default)
            {
                if (GroupPolicies().GetState(TogglePolicy::Policy::DefaultSource) == PolicyState::Enabled &&
                    source.Identifier == s_Source_WingetCommunityDefault_Identifier)
                {
                    throw GroupPolicyException(TogglePolicy::Policy::DefaultSource);
                }

                if (GroupPolicies().GetState(TogglePolicy::Policy::MSStoreSource) == PolicyState::Enabled &&
                    source.Identifier == s_Source_WingetMSStoreDefault_Identifier)
                {
                    throw GroupPolicyException(TogglePolicy::Policy::MSStoreSource);
                }
            }
        }

        // Attempts to read a single scalar value from the node.
        template<typename Value>
        bool TryReadScalar(std::string_view settingName, const std::string& settingValue, const YAML::Node& sourceNode, std::string_view name, Value& value, bool required = true)
        {
            YAML::Node valueNode = sourceNode[std::string{ name }];

            if (!valueNode || !valueNode.IsScalar())
            {
                if (required)
                {
                    AICLI_LOG(Repo, Error, << "Setting '" << settingName << "' did not contain the expected format (" << name << " is invalid within a source):\n" << settingValue);
                }
                return false;
            }

            value = valueNode.as<Value>();
            return true;
        }

        // Attempts to read the source details from the given stream.
        // Results are all or nothing; if any failures occur, no details are returned.
        bool TryReadSourceDetails(
            std::string_view settingName,
            std::istream& stream,
            std::string_view rootName,
            std::function<bool(SourceDetailsInternal&, const std::string&, const YAML::Node&)> parse,
            std::vector<SourceDetailsInternal>& sourceDetails)
        {
            std::vector<SourceDetailsInternal> result;
            std::string settingValue = Utility::ReadEntireStream(stream);

            YAML::Node document;
            try
            {
                document = YAML::Load(settingValue);
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(YAML, Error, << "Setting '" << settingName << "' contained invalid YAML (" << e.what() << "):\n" << settingValue);
                return false;
            }

            try
            {
                YAML::Node sources = document[rootName];
                if (!sources)
                {
                    AICLI_LOG(Repo, Error, << "Setting '" << settingName << "' did not contain the expected format (missing " << rootName << "):\n" << settingValue);
                    return false;
                }

                if (sources.IsNull())
                {
                    // An empty sources is an acceptable thing.
                    return true;
                }

                if (!sources.IsSequence())
                {
                    AICLI_LOG(Repo, Error, << "Setting '" << settingName << "' did not contain the expected format (" << rootName << " was not a sequence):\n" << settingValue);
                    return false;
                }

                for (const auto& source : sources.Sequence())
                {
                    SourceDetailsInternal details;
                    if (!parse(details, settingValue, source))
                    {
                        return false;
                    }

                    result.emplace_back(std::move(details));
                }
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(YAML, Error, << "Setting '" << settingName << "' contained unexpected YAML (" << e.what() << "):\n" << settingValue);
                return false;
            }

            sourceDetails = std::move(result);
            return true;
        }

        // Gets the source details from a particular setting, or an empty optional if no setting exists.
        std::optional<std::vector<SourceDetailsInternal>> TryGetSourcesFromSetting(
            const Settings::StreamDefinition& setting,
            std::string_view rootName,
            std::function<bool(SourceDetailsInternal&, const std::string&, const YAML::Node&)> parse)
        {
            auto sourcesStream = Settings::GetSettingStream(setting);
            if (!sourcesStream)
            {
                // Note that this case is different than the one in which all sources have been removed.
                return {};
            }
            else
            {
                std::vector<SourceDetailsInternal> result;
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCES_INVALID, !TryReadSourceDetails(setting.Path, *sourcesStream, rootName, parse, result));
                return result;
            }
        }

        // Gets the source details from a particular setting.
        std::vector<SourceDetailsInternal> GetSourcesFromSetting(
            const Settings::StreamDefinition& setting,
            std::string_view rootName,
            std::function<bool(SourceDetailsInternal&, const std::string&, const YAML::Node&)> parse)
        {
            return TryGetSourcesFromSetting(setting, rootName, parse).value_or(std::vector<SourceDetailsInternal>{});
        }

        // Gets the metadata.
        std::vector<SourceDetailsInternal> GetMetadata()
        {
            return GetSourcesFromSetting(
                Settings::Streams::SourcesMetadata,
                s_MetadataYaml_Sources,
                [&](SourceDetailsInternal& details, const std::string& settingValue, const YAML::Node& source)
                {
                    std::string_view name = Settings::Streams::SourcesMetadata.Path;
                    if (!TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_Name, details.Name)) { return false; }
                    int64_t lastUpdateInEpoch{};
                    if (!TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_LastUpdate, lastUpdateInEpoch)) { return false; }
                    details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(lastUpdateInEpoch);
                    return true;
                });
        }

        // Checks whether a default source is enabled with the current settings
        bool IsDefaultSourceEnabled(std::string_view sourceToLog, ExperimentalFeature::Feature feature, TogglePolicy::Policy policy)
        {
            if (!ExperimentalFeature::IsEnabled(feature))
            {
                // No need to log here
                return false;
            }

            if (!GroupPolicies().IsEnabled(policy))
            {
                AICLI_LOG(Repo, Info, << "The default source " << sourceToLog << " is disabled due to Group Policy");
                return false;
            }

            return true;
        }

        // Gets the sources from a particular origin.
        std::vector<SourceDetailsInternal> GetSourcesByOrigin(SourceOrigin origin)
        {
            std::vector<SourceDetailsInternal> result;

            switch (origin)
            {
            case SourceOrigin::Default:
            {
                if (IsWingetCommunityDefaultSourceEnabled())
                {
                    SourceDetailsInternal details;
                    details.Name = s_Source_WingetCommunityDefault_Name;
                    details.Type = Microsoft::PreIndexedPackageSourceFactory::Type();
                    details.Arg = s_Source_WingetCommunityDefault_Arg;
                    details.Data = s_Source_WingetCommunityDefault_Data;
                    details.Identifier = s_Source_WingetCommunityDefault_Identifier;
                    details.TrustLevel = SourceTrustLevel::Trusted;
                    result.emplace_back(std::move(details));
                }

                if (IsWingetMSStoreDefaultSourceEnabled())
                {
                    SourceDetailsInternal details;
                    details.Name = s_Source_WingetMSStoreDefault_Name;
                    details.Type = Microsoft::PreIndexedPackageSourceFactory::Type();
                    details.Arg = s_Source_WingetMSStoreDefault_Arg;
                    details.Data = s_Source_WingetMSStoreDefault_Data;
                    details.Identifier = s_Source_WingetMSStoreDefault_Identifier;
                    details.TrustLevel = SourceTrustLevel::Trusted;
                    result.emplace_back(std::move(details));
                }
            }
            break;
            case SourceOrigin::User:
            {
                std::vector<SourceDetailsInternal> userSources = GetSourcesFromSetting(
                    Settings::Streams::UserSources,
                    s_SourcesYaml_Sources,
                    [&](SourceDetailsInternal& details, const std::string& settingValue, const YAML::Node& source)
                    {
                        std::string_view name = Settings::Streams::UserSources.Path;
                        if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Name, details.Name)) { return false; }
                        if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Type, details.Type)) { return false; }
                        if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Arg, details.Arg)) { return false; }
                        if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Data, details.Data)) { return false; }
                        if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_IsTombstone, details.IsTombstone)) { return false; }
                        TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Identifier, details.Identifier);
                        return true;
                    });

                for (auto& source : userSources)
                {
                    if (Utility::CaseInsensitiveEquals(Rest::RestSourceFactory::Type(), source.Type)
                        && !Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::ExperimentalRestSource))
                    {
                        continue;
                    }

                    // Check source against list of allowed sources and drop tombstones for required sources
                    if (!IsUserSourceAllowedByPolicy(source.Name, source.Type, source.Arg, source.IsTombstone))
                    {
                        AICLI_LOG(Repo, Warning, << "User source " << source.Name << " dropped because of group policy");
                        continue;
                    }

                    result.emplace_back(std::move(source));
                }
            }
            break;
            case SourceOrigin::GroupPolicy:
            {
                if (GroupPolicies().GetState(TogglePolicy::Policy::AdditionalSources) == PolicyState::Enabled)
                {
                    auto additionalSourcesOpt = GroupPolicies().GetValueRef<ValuePolicy::AdditionalSources>();
                    if (additionalSourcesOpt.has_value())
                    {
                        const auto& additionalSources = additionalSourcesOpt->get();
                        for (const auto& additionalSource : additionalSources)
                        {
                            SourceDetailsInternal details;
                            details.Name = additionalSource.Name;
                            details.Type = additionalSource.Type;
                            details.Arg = additionalSource.Arg;
                            details.Data = additionalSource.Data;
                            details.Identifier = additionalSource.Identifier;
                            details.Origin = SourceOrigin::GroupPolicy;
                            result.emplace_back(std::move(details));
                        }
                    }
                }
            }
            break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            for (auto& source : result)
            {
                source.Origin = origin;
            }

            return result;
        }

        // Sets the sources for a particular setting, from a particular origin.
        void SetSourcesToSettingWithFilter(const Settings::StreamDefinition& setting, SourceOrigin origin, const std::vector<SourceDetailsInternal>& sources)
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << s_SourcesYaml_Sources;
            out << YAML::BeginSeq;

            for (const auto& details : sources)
            {
                if (details.Origin == origin)
                {
                    out << YAML::BeginMap;
                    out << YAML::Key << s_SourcesYaml_Source_Name << YAML::Value << details.Name;
                    out << YAML::Key << s_SourcesYaml_Source_Type << YAML::Value << details.Type;
                    out << YAML::Key << s_SourcesYaml_Source_Arg << YAML::Value << details.Arg;
                    out << YAML::Key << s_SourcesYaml_Source_Data << YAML::Value << details.Data;
                    out << YAML::Key << s_SourcesYaml_Source_Identifier << YAML::Value << details.Identifier;
                    out << YAML::Key << s_SourcesYaml_Source_IsTombstone << YAML::Value << details.IsTombstone;
                    out << YAML::EndMap;
                }
            }

            out << YAML::EndSeq;
            out << YAML::EndMap;

            Settings::SetSetting(setting, out.str());
        }

        // Sets the metadata only (which is not a secure setting and can be set unprivileged)
        void SetMetadata(const std::vector<SourceDetailsInternal>& sources)
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << s_MetadataYaml_Sources;
            out << YAML::BeginSeq;

            for (const auto& details : sources)
            {
                out << YAML::BeginMap;
                out << YAML::Key << s_MetadataYaml_Source_Name << YAML::Value << details.Name;
                out << YAML::Key << s_MetadataYaml_Source_LastUpdate << YAML::Value << Utility::ConvertSystemClockToUnixEpoch(details.LastUpdateTime);
                out << YAML::EndMap;
            }

            out << YAML::EndSeq;
            out << YAML::EndMap;

            Settings::SetSetting(Settings::Streams::SourcesMetadata, out.str());
        }

        // Sets the sources for a given origin.
        void SetSourcesByOrigin(SourceOrigin origin, const std::vector<SourceDetailsInternal>& sources)
        {
            switch (origin)
            {
            case SourceOrigin::User:
                SetSourcesToSettingWithFilter(Settings::Streams::UserSources, SourceOrigin::User, sources);
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            SetMetadata(sources);
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        static std::map<std::string, std::function<std::unique_ptr<ISourceFactory>()>> s_Sources_TestHook_SourceFactories;
#endif

        std::unique_ptr<ISourceFactory> GetFactoryForType(std::string_view type)
        {
#ifndef AICLI_DISABLE_TEST_HOOKS
            // Tests can ensure case matching
            auto itr = s_Sources_TestHook_SourceFactories.find(std::string(type));
            if (itr != s_Sources_TestHook_SourceFactories.end())
            {
                return itr->second();
            }
#endif

            // For now, enable an empty type to represent the only one we have.
            if (type.empty() ||
                Utility::CaseInsensitiveEquals(Microsoft::PreIndexedPackageSourceFactory::Type(), type))
            {
                return Microsoft::PreIndexedPackageSourceFactory::Create();
            }
            // Should always come from code, so no need for case insensitivity
            else if (Microsoft::PredefinedInstalledSourceFactory::Type() == type)
            {
                return Microsoft::PredefinedInstalledSourceFactory::Create();
            }
            else if (Utility::CaseInsensitiveEquals(Rest::RestSourceFactory::Type(), type)
                && Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::ExperimentalRestSource))
            {
                return Rest::RestSourceFactory::Create();
            }

            THROW_HR(APPINSTALLER_CLI_ERROR_INVALID_SOURCE_TYPE);
        }

        std::shared_ptr<ISource> CreateSourceFromDetails(const SourceDetails& details, IProgressCallback& progress)
        {
            return GetFactoryForType(details.Type)->Create(details, progress);
        }

        template <typename MemberFunc>
        void AddOrUpdateFromDetails(SourceDetails& details, MemberFunc member, IProgressCallback& progress)
        {
            auto factory = GetFactoryForType(details.Type);

            // Attempt; if it fails, wait a short time and retry.
            try
            {
                (factory.get()->*member)(details, progress);
                details.LastUpdateTime = std::chrono::system_clock::now();
                return;
            }
            CATCH_LOG();

            AICLI_LOG(Repo, Info, << "Source add/update failed, waiting a bit and retrying: " << details.Name);
            std::this_thread::sleep_for(2s);

            // If this one fails, maybe the problem is persistent.
            (factory.get()->*member)(details, progress);
            details.LastUpdateTime = std::chrono::system_clock::now();
        }

        void AddSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            AddOrUpdateFromDetails(details, &ISourceFactory::Add, progress);
        }

        void UpdateSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            AddOrUpdateFromDetails(details, &ISourceFactory::Update, progress);
        }

        void RemoveSourceFromDetails(const SourceDetails& details, IProgressCallback& progress)
        {
            auto factory = GetFactoryForType(details.Type);

            factory->Remove(details, progress);
        }

        // Determines whether (and logs why) a source should be updated before it is opened.
        bool ShouldUpdateBeforeOpen(const SourceDetails& details)
        {
            constexpr static auto s_ZeroMins = 0min;
            auto autoUpdateTime = User().Get<Setting::AutoUpdateTimeInMinutes>();

            // A value of zero means no auto update, to get update the source run `winget update`
            if (autoUpdateTime != s_ZeroMins)
            {
                auto autoUpdateTimeMins = std::chrono::minutes(autoUpdateTime);
                auto timeSinceLastUpdate = std::chrono::system_clock::now() - details.LastUpdateTime;
                if (timeSinceLastUpdate > autoUpdateTimeMins)
                {
                    AICLI_LOG(Repo, Info, << "Source past auto update time [" <<
                        std::chrono::duration_cast<std::chrono::minutes>(autoUpdateTimeMins).count() << " mins]; it has been at least " <<
                        std::chrono::duration_cast<std::chrono::minutes>(timeSinceLastUpdate).count() << " mins");
                    return true;
                }
            }

            return false;
        }

        // Struct containing internal implementation of source list
        // This contains all sources including tombstoned sources
        struct SourceListInternal
        {
            SourceListInternal();

            // Get a list of current sources references which can be used to update the contents in place.
            // e.g. update the LastTimeUpdated value of sources.
            std::vector<std::reference_wrapper<SourceDetailsInternal>> GetCurrentSourceRefs();

            // Current source means source that's not in tombstone
            SourceDetailsInternal* GetCurrentSource(std::string_view name);

            // Source includes ones in tombstone
            SourceDetailsInternal* GetSource(std::string_view name);

            // Add/remove a current source
            void AddSource(const SourceDetailsInternal& source);
            void RemoveSource(const SourceDetailsInternal& source);

            // Save source metadata. Currently only LastTimeUpdated is used.
            void SaveMetadata() const;

        private:
            std::vector<SourceDetailsInternal> m_sourceList;

            // calls std::find_if and return the iterator.
            auto FindSource(std::string_view name, bool includeTombstone = false);
        };

        SourceListInternal::SourceListInternal()
        {
            for (SourceOrigin origin : { SourceOrigin::GroupPolicy, SourceOrigin::User, SourceOrigin::Default })
            {
                auto forOrigin = GetSourcesByOrigin(origin);

                for (auto&& source : forOrigin)
                {
                    auto foundSource = GetSource(source.Name);
                    if (!foundSource)
                    {
                        // Name not already defined, add it
                        m_sourceList.emplace_back(std::move(source));
                    }
                    else
                    {
                        AICLI_LOG(Repo, Info, << "Source named '" << foundSource->Name << "' is already defined at origin " << ToString(foundSource->Origin) <<
                            ". The source from origin " << ToString(origin) << " is dropped.");
                    }
                }
            }

            auto metadata = GetMetadata();
            for (const auto& metaSource : metadata)
            {
                auto source = GetSource(metaSource.Name);
                if (source)
                {
                    source->LastUpdateTime = metaSource.LastUpdateTime;
                }
            }
        }

        std::vector<std::reference_wrapper<SourceDetailsInternal>> SourceListInternal::GetCurrentSourceRefs()
        {
            std::vector<std::reference_wrapper<SourceDetailsInternal>> result;

            for (auto& s : m_sourceList)
            {
                if (!s.IsTombstone)
                {
                    result.emplace_back(std::ref(s));
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "GetCurrentSourceRefs: Source named '" << s.Name << "' from origin " << ToString(s.Origin) << " is a tombstone and is dropped.");
                }
            }

            return result;
        }

        auto SourceListInternal::FindSource(std::string_view name, bool includeTombstone)
        {
            return std::find_if(m_sourceList.begin(), m_sourceList.end(),
                [name, includeTombstone](const SourceDetailsInternal& sd)
                {
                    return Utility::ICUCaseInsensitiveEquals(sd.Name, name) &&
                        (!sd.IsTombstone || includeTombstone);
                });
        }

        SourceDetailsInternal* SourceListInternal::GetCurrentSource(std::string_view name)
        {
            auto itr = FindSource(name);
            return itr == m_sourceList.end() ? nullptr : &(*itr);
        }

        SourceDetailsInternal* SourceListInternal::GetSource(std::string_view name)
        {
            auto itr = FindSource(name, true);
            return itr == m_sourceList.end() ? nullptr : &(*itr);
        }

        void SourceListInternal::AddSource(const SourceDetailsInternal& details)
        {
            // Erase the source's tombstone entry if applicable
            auto itr = FindSource(details.Name, true);
            if (itr != m_sourceList.end())
            {
                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_INVALID_STATE), !itr->IsTombstone);
                m_sourceList.erase(itr);
            }

            m_sourceList.emplace_back(details);

            SetSourcesByOrigin(SourceOrigin::User, m_sourceList);
        }

        void SourceListInternal::RemoveSource(const SourceDetailsInternal& source)
        {
            switch (source.Origin)
            {
            case SourceOrigin::Default:
            {
                SourceDetailsInternal tombstone;
                tombstone.Name = source.Name;
                tombstone.IsTombstone = true;
                tombstone.Origin = SourceOrigin::User;
                m_sourceList.emplace_back(std::move(tombstone));
            }
            break;
            case SourceOrigin::User:
                m_sourceList.erase(FindSource(source.Name));
                break;
            case SourceOrigin::GroupPolicy:
                // This should have already been blocked higher up.
                AICLI_LOG(Repo, Error, << "Attempting to remove Group Policy source: " << source.Name);
                THROW_HR(E_UNEXPECTED);
            default:
                THROW_HR(E_UNEXPECTED);
            }

            SetSourcesByOrigin(SourceOrigin::User, m_sourceList);
        }

        void SourceListInternal::SaveMetadata() const
        {
            SetMetadata(m_sourceList);
        }
    }

    std::string_view ToString(SourceOrigin origin)
    {
        switch (origin)
        {
        case SourceOrigin::Default:
            return "Default"sv;
        case SourceOrigin::User:
            return "User"sv;
        case SourceOrigin::GroupPolicy:
            return "GroupPolicy"sv;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<SourceDetails> GetSources()
    {
        SourceListInternal sourceList;

        std::vector<SourceDetails> result;
        for (auto&& source : sourceList.GetCurrentSourceRefs())
        {
            result.emplace_back(std::move(source));
        }

        return result;
    }

    std::optional<SourceDetails> GetSource(std::string_view name)
    {
        // Check all sources for the given name.
        SourceListInternal sourceList;

        auto source = sourceList.GetCurrentSource(name);
        if (!source)
        {
            return {};
        }
        else
        {
            return *source;
        }
    }

    void AddSource(std::string_view name, std::string_view type, std::string_view arg, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        AICLI_LOG(Repo, Info, << "Adding source: Name[" << name << "], Type[" << type << "], Arg[" << arg << "]");

        // Check all sources for the given name.
        SourceListInternal sourceList;

        auto source = sourceList.GetCurrentSource(name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, source != nullptr);

        // Check sources allowed by group policy
        auto blockingPolicy = GetPolicyBlockingUserSource(name, type, arg, false);
        if (blockingPolicy != TogglePolicy::Policy::None)
        {
            throw GroupPolicyException(blockingPolicy);
        }

        SourceDetailsInternal details;
        details.Name = name;
        details.Type = type;
        details.Arg = arg;
        details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(0);
        details.Origin = SourceOrigin::User;

        // Check feature flag enablement for rest source.
        if (Utility::CaseInsensitiveEquals(Rest::RestSourceFactory::Type(), type)
            && !Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::ExperimentalRestSource))
        {
            AICLI_LOG(Repo, Error, << Settings::ExperimentalFeature::GetFeature(Settings::ExperimentalFeature::Feature::ExperimentalRestSource).Name()
                << " feature is disabled. Execution cancelled.");
            THROW_HR(APPINSTALLER_CLI_ERROR_EXPERIMENTAL_FEATURE_DISABLED);
        }

        AddSourceFromDetails(details, progress);

        AICLI_LOG(Repo, Info, << "Source created with extra data: " << details.Data);
        AICLI_LOG(Repo, Info, << "Source created with identifier: " << details.Identifier);

        sourceList.AddSource(details);
    }

    OpenSourceResult OpenSource(std::string_view name, IProgressCallback& progress)
    {
        SourceListInternal sourceList;
        auto currentSources = sourceList.GetCurrentSourceRefs();

        if (name.empty())
        {
            if (currentSources.empty())
            {
                AICLI_LOG(Repo, Info, << "Default source requested, but no sources configured");
                return {};
            }
            else if (currentSources.size() == 1)
            {
                AICLI_LOG(Repo, Info, << "Default source requested, only 1 source available, using the only source: " << currentSources[0].get().Name);
                return OpenSource(currentSources[0].get().Name, progress);
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Default source requested, multiple sources available, creating aggregated source.");
                auto aggregatedSource = std::make_shared<CompositeSource>("*DefaultSource");
                OpenSourceResult result;

                bool sourceUpdated = false;
                for (auto& source : currentSources)
                {
                    AICLI_LOG(Repo, Info, << "Adding to aggregated source: " << source.get().Name);

                    if (ShouldUpdateBeforeOpen(source))
                    {
                        try
                        {
                            // TODO: Consider adding a context callback to indicate we are doing the same action
                            // to avoid the progress bar fill up multiple times.
                            UpdateSourceFromDetails(source, progress);
                            sourceUpdated = true;
                        }
                        catch (...)
                        {
                            AICLI_LOG(Repo, Warning, << "Failed to update source: " << source.get().Name);
                            result.SourcesWithUpdateFailure.emplace_back(source);
                        }
                    }
                    aggregatedSource->AddAvailableSource(CreateSourceFromDetails(source, progress));
                }

                if (sourceUpdated)
                {
                    sourceList.SaveMetadata();
                }

                result.Source = aggregatedSource;
                return result;
            }
        }
        else
        {
            auto source = sourceList.GetCurrentSource(name);
            if (!source)
            {
                AICLI_LOG(Repo, Info, << "Named source requested, but not found: " << name);
                return {};
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Named source requested, found: " << source->Name);

                OpenSourceResult result;

                if (ShouldUpdateBeforeOpen(*source))
                {
                    try
                    {
                        UpdateSourceFromDetails(*source, progress);
                        sourceList.SaveMetadata();
                    }
                    catch (...)
                    {
                        AICLI_LOG(Repo, Warning, << "Failed to update source: " << (*source).Name);
                        result.SourcesWithUpdateFailure.emplace_back(*source);
                    }
                }

                result.Source = CreateSourceFromDetails(*source, progress);
                return result;
            }
        }
    }

    std::shared_ptr<ISource> OpenPredefinedSource(PredefinedSource source, IProgressCallback& progress)
    {
        SourceDetails details;
        details.Origin = SourceOrigin::Predefined;

        switch (source)
        {
        case PredefinedSource::Installed:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::None);
            return CreateSourceFromDetails(details, progress);
        case PredefinedSource::ARP:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::ARP);
            return CreateSourceFromDetails(details, progress);
        case PredefinedSource::MSIX:
            details.Type = Microsoft::PredefinedInstalledSourceFactory::Type();
            details.Arg = Microsoft::PredefinedInstalledSourceFactory::FilterToString(Microsoft::PredefinedInstalledSourceFactory::Filter::MSIX);
            return CreateSourceFromDetails(details, progress);
        }

        THROW_HR(E_UNEXPECTED);
    }

    std::shared_ptr<ISource> CreateCompositeSource(const std::shared_ptr<ISource>& installedSource, const std::shared_ptr<ISource>& availableSource, CompositeSearchBehavior searchBehavior)
    {
        std::shared_ptr<CompositeSource> result = std::dynamic_pointer_cast<CompositeSource>(availableSource);

        if (!result)
        {
            result = std::make_shared<CompositeSource>("*CompositeSource");
            result->AddAvailableSource(availableSource);
        }

        result->SetInstalledSource(installedSource, searchBehavior);

        return result;
    }

    bool UpdateSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        SourceListInternal sourceList;

        auto source = sourceList.GetCurrentSource(name);
        if (!source)
        {
            AICLI_LOG(Repo, Info, << "Named source to be updated, but not found: " << name);
            return false;
        }
        else
        {
            AICLI_LOG(Repo, Info, << "Named source to be updated, found: " << source->Name);

            UpdateSourceFromDetails(*source, progress);

            sourceList.SaveMetadata();
            return true;
        }
    }

    bool RemoveSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        SourceListInternal sourceList;

        auto source = sourceList.GetCurrentSource(name);
        if (!source)
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, but not found: " << name);
            return false;
        }
        else
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, found: " << source->Name << " [" << ToString(source->Origin) << ']');

            EnsureSourceIsRemovable(*source);
            RemoveSourceFromDetails(*source, progress);
            sourceList.RemoveSource(*source);

            return true;
        }
    }

    bool DropSource(std::string_view name)
    {
        if (name.empty())
        {
            Settings::RemoveSetting(Settings::Streams::UserSources);
            Settings::RemoveSetting(Settings::Streams::SourcesMetadata);
            return true;
        }
        else
        {
            SourceListInternal sourceList;

            auto source = sourceList.GetCurrentSource(name);
            if (!source)
            {
                AICLI_LOG(Repo, Info, << "Named source to be dropped, but not found: " << name);
                return false;
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Named source to be dropped, found: " << source->Name);

                EnsureSourceIsRemovable(*source);
                sourceList.RemoveSource(*source);

                return true;
            }
        }
    }

    bool SearchRequest::IsForEverything() const
    {
        return (!Query.has_value() && Inclusions.empty() && Filters.empty());
    }

    std::string SearchRequest::ToString() const
    {
        std::ostringstream result;

        result << "Query:";
        if (Query)
        {
            result << '\'' << Query.value().Value << "'[" << MatchTypeToString(Query.value().Type) << ']';
        }
        else
        {
            result << "[none]";
        }

        for (const auto& include : Inclusions)
        {
            result << " Inclusions:" << PackageMatchFieldToString(include.Field) << "='" << include.Value << "'[" << MatchTypeToString(include.Type) << "]";
        }

        for (const auto& filter : Filters)
        {
            result << " Filter:" << PackageMatchFieldToString(filter.Field) << "='" << filter.Value << "'[" << MatchTypeToString(filter.Type) << "]";
        }

        if (MaximumResults)
        {
            result << " Limit:" << MaximumResults;
        }

        return result.str();
    }

    std::string_view ToString(PackageVersionMetadata pvm)
    {
        switch (pvm)
        {
        case PackageVersionMetadata::InstalledType: return "InstalledType"sv;
        case PackageVersionMetadata::InstalledScope: return "InstalledScope"sv;
        case PackageVersionMetadata::InstalledLocation: return "InstalledLocation"sv;
        case PackageVersionMetadata::StandardUninstallCommand: return "StandardUninstallCommand"sv;
        case PackageVersionMetadata::SilentUninstallCommand: return "SilentUninstallCommand"sv;
        default: return "Unknown"sv;
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void TestHook_SetSourceFactoryOverride(const std::string& type, std::function<std::unique_ptr<ISourceFactory>()>&& factory)
    {
        s_Sources_TestHook_SourceFactories[type] = std::move(factory);
    }

    void TestHook_ClearSourceFactoryOverrides()
    {
        s_Sources_TestHook_SourceFactories.clear();
    }
#endif
}
