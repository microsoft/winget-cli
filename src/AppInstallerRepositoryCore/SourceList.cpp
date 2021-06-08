// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceList.h"


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
            Settings::Stream& stream,
            std::string_view rootName,
            std::function<bool(SourceDetailsInternal&, const std::string&, const YAML::Node&)> parse)
        {
            auto sourcesStream = stream.Get();
            if (!sourcesStream)
            {
                // Note that this case is different than the one in which all sources have been removed.
                return {};
            }
            else
            {
                std::vector<SourceDetailsInternal> result;
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCES_INVALID, !TryReadSourceDetails(stream.Definition().Path, *sourcesStream, rootName, parse, result));
                return result;
            }
        }

        // Gets the source details from a particular setting.
        std::vector<SourceDetailsInternal> GetSourcesFromSetting(
            Settings::Stream& stream,
            std::string_view rootName,
            std::function<bool(SourceDetailsInternal&, const std::string&, const YAML::Node&)> parse)
        {
            return TryGetSourcesFromSetting(stream, rootName, parse).value_or(std::vector<SourceDetailsInternal>{});
        }

        // Gets the metadata.
        std::vector<SourceDetailsInternal> GetMetadata(Settings::Stream& stream)
        {
            return GetSourcesFromSetting(
                stream,
                s_MetadataYaml_Sources,
                [&](SourceDetailsInternal& details, const std::string& settingValue, const YAML::Node& source)
                {
                    std::string_view name = Settings::Stream::SourcesMetadata.Path;
                    if (!TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_Name, details.Name)) { return false; }
                    int64_t lastUpdateInEpoch{};
                    if (!TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_LastUpdate, lastUpdateInEpoch)) { return false; }
                    details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(lastUpdateInEpoch);
                    return true;
                });
        }

        // Gets the sources from a particular origin.
        std::vector<SourceDetailsInternal> GetSourcesByOrigin(SourceOrigin origin, Settings::Stream& userSettingStream)
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
                    details.TrustLevel = SourceTrustLevel::Trusted | SourceTrustLevel::StoreOrigin;
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
                    details.TrustLevel = SourceTrustLevel::Trusted | SourceTrustLevel::StoreOrigin;
                    result.emplace_back(std::move(details));
                }
            }
            break;
            case SourceOrigin::User:
            {
                std::vector<SourceDetailsInternal> userSources = GetSourcesFromSetting(
                    userSettingStream,
                    s_SourcesYaml_Sources,
                    [&](SourceDetailsInternal& details, const std::string& settingValue, const YAML::Node& source)
                    {
                        std::string_view name = Settings::Stream::UserSources.Path;
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
        bool SetSourcesToSettingWithFilter(Settings::Stream& stream, SourceOrigin origin, const std::vector<SourceDetailsInternal>& sources)
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

            return stream.Set(out.str());
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

            Settings::SetSetting(Settings::Stream::SourcesMetadata, out.str());
        }

        // Sets the sources for a given origin.
        bool SetSourcesByOrigin(SourceOrigin origin, Settings::Stream& userSettingStream, const std::vector<SourceDetailsInternal>& sources)
        {
            switch (origin)
            {
            case SourceOrigin::User:
                if (!SetSourcesToSettingWithFilter(userSettingStream, SourceOrigin::User, sources))
                {
                    return false;
                }
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            return SetMetadata(sources);
        }
    }

    SourceList::SourceList() : m_userSourcesStream(Settings::Stream::UserSources), m_metadataStream(Settings::Stream::SourcesMetadata)
    {
        for (SourceOrigin origin : { SourceOrigin::GroupPolicy, SourceOrigin::User, SourceOrigin::Default })
        {
            auto forOrigin = GetSourcesByOrigin(origin, m_userSourcesStream);

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

        auto metadata = GetMetadata(m_metadataStream);
        for (const auto& metaSource : metadata)
        {
            auto source = GetSource(metaSource.Name);
            if (source)
            {
                source->LastUpdateTime = metaSource.LastUpdateTime;
            }
        }
    }

    std::vector<std::reference_wrapper<SourceDetailsInternal>> SourceList::GetCurrentSourceRefs()
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

    auto SourceList::FindSource(std::string_view name, bool includeTombstone)
    {
        return std::find_if(m_sourceList.begin(), m_sourceList.end(),
            [name, includeTombstone](const SourceDetailsInternal& sd)
            {
                return Utility::ICUCaseInsensitiveEquals(sd.Name, name) &&
                    (!sd.IsTombstone || includeTombstone);
            });
    }

    SourceDetailsInternal* SourceList::GetCurrentSource(std::string_view name)
    {
        auto itr = FindSource(name);
        return itr == m_sourceList.end() ? nullptr : &(*itr);
    }

    SourceDetailsInternal* SourceList::GetSource(std::string_view name)
    {
        auto itr = FindSource(name, true);
        return itr == m_sourceList.end() ? nullptr : &(*itr);
    }

    void SourceList::AddSource(const SourceDetailsInternal& details)
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

    void SourceList::RemoveSource(const SourceDetailsInternal& source)
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

    void SourceList::SaveMetadata() const
    {
        SetMetadata(m_sourceList);
    }
}
