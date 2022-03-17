// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceList.h"
#include "SourcePolicy.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"
#include "Rest/RestSourceFactory.h"

using namespace AppInstaller::Settings;
using namespace std::string_view_literals;

namespace AppInstaller::Repository
{
    namespace
    {
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
        constexpr std::string_view s_MetadataYaml_Source_AcceptedAgreementsIdentifier = "AcceptedAgreementsIdentifier"sv;
        constexpr std::string_view s_MetadataYaml_Source_AcceptedAgreementFields = "AcceptedAgreementFields"sv;

        constexpr std::string_view s_Source_WingetCommunityDefault_Name = "winget"sv;
        constexpr std::string_view s_Source_WingetCommunityDefault_Arg = "https://cdn.winget.microsoft.com/cache"sv;
        constexpr std::string_view s_Source_WingetCommunityDefault_Data = "Microsoft.Winget.Source_8wekyb3d8bbwe"sv;
        constexpr std::string_view s_Source_WingetCommunityDefault_Identifier = "Microsoft.Winget.Source_8wekyb3d8bbwe"sv;

        constexpr std::string_view s_Source_MSStoreDefault_Name = "msstore"sv;
        constexpr std::string_view s_Source_MSStoreDefault_Arg = "https://storeedgefd.dsx.mp.microsoft.com/v9.0"sv;
        constexpr std::string_view s_Source_MSStoreDefault_Identifier = "StoreEdgeFD"sv;

        constexpr std::string_view s_Source_DesktopFrameworks_Name = "microsoft.builtin.desktop.frameworks"sv;
        constexpr std::string_view s_Source_DesktopFrameworks_Arg = "https://cdn.winget.microsoft.com/platform"sv;
        constexpr std::string_view s_Source_DesktopFrameworks_Data = "Microsoft.Winget.Platform.Source_8wekyb3d8bbwe"sv;
        constexpr std::string_view s_Source_DesktopFrameworks_Identifier = "Microsoft.Winget.Platform.Source_8wekyb3d8bbwe"sv;

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
            Settings::Stream& setting,
            std::string_view rootName,
            std::function<bool(SourceDetailsInternal&, const std::string&, const YAML::Node&)> parse)
        {
            auto sourcesStream = setting.Get();
            if (!sourcesStream)
            {
                // Note that this case is different than the one in which all sources have been removed.
                return {};
            }
            else
            {
                std::vector<SourceDetailsInternal> result;
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCES_INVALID, !TryReadSourceDetails(setting.GetName(), *sourcesStream, rootName, parse, result));
                return result;
            }
        }

        // Gets the source details from a particular setting.
        std::vector<SourceDetailsInternal> GetSourcesFromSetting(
            Settings::Stream& setting,
            std::string_view rootName,
            std::function<bool(SourceDetailsInternal&, const std::string&, const YAML::Node&)> parse)
        {
            return TryGetSourcesFromSetting(setting, rootName, parse).value_or(std::vector<SourceDetailsInternal>{});
        }

        // Sets the sources for a particular setting, from a particular origin.
        [[nodiscard]] bool SetSourcesToSettingWithFilter(Settings::Stream& setting, SourceOrigin origin, const std::vector<SourceDetailsInternal>& sources)
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

            return setting.Set(out.str());
        }

        // Assumes that names match already
        bool DoSourceDetailsInternalMatch(const SourceDetailsInternal& left, const SourceDetailsInternal& right)
        {
            return left.Arg == right.Arg &&
                left.Identifier == right.Identifier &&
                Utility::CaseInsensitiveEquals(left.Type, right.Type);
        }

        bool ShouldBeHidden(const SourceDetailsInternal& details)
        {
            return details.IsTombstone || details.Origin == SourceOrigin::Metadata || !details.IsVisible;
        }
    }

    void SourceDetailsInternal::CopyMetadataFieldsTo(SourceDetailsInternal& target)
    {
        target.LastUpdateTime = LastUpdateTime;
        target.AcceptedAgreementFields = AcceptedAgreementFields;
        target.AcceptedAgreementsIdentifier = AcceptedAgreementsIdentifier;
    }

    std::string_view GetWellKnownSourceName(WellKnownSource source)
    {
        switch (source)
        {
        case WellKnownSource::WinGet:
            return s_Source_WingetCommunityDefault_Name;
        case WellKnownSource::MicrosoftStore:
            return s_Source_MSStoreDefault_Name;
        case WellKnownSource::DesktopFrameworks:
            return s_Source_DesktopFrameworks_Name;
        }

        return {};
    }

    std::string_view GetWellKnownSourceArg(WellKnownSource source)
    {
        switch (source)
        {
        case WellKnownSource::WinGet:
            return s_Source_WingetCommunityDefault_Arg;
        case WellKnownSource::MicrosoftStore:
            return s_Source_MSStoreDefault_Arg;
        case WellKnownSource::DesktopFrameworks:
            return s_Source_DesktopFrameworks_Arg;
        }

        return {};
    }

    std::string_view GetWellKnownSourceIdentifier(WellKnownSource source)
    {
        switch (source)
        {
        case WellKnownSource::WinGet:
            return s_Source_WingetCommunityDefault_Identifier;
        case WellKnownSource::MicrosoftStore:
            return s_Source_MSStoreDefault_Identifier;
        case WellKnownSource::DesktopFrameworks:
            return s_Source_DesktopFrameworks_Identifier;
        }

        return {};
    }

    SourceDetailsInternal GetWellKnownSourceDetailsInternal(WellKnownSource source)
    {
        switch (source)
        {
        case WellKnownSource::WinGet:
        {
            SourceDetailsInternal details;
            details.Origin = SourceOrigin::Default;
            details.Name = s_Source_WingetCommunityDefault_Name;
            details.Type = Microsoft::PreIndexedPackageSourceFactory::Type();
            details.Arg = s_Source_WingetCommunityDefault_Arg;
            details.Data = s_Source_WingetCommunityDefault_Data;
            details.Identifier = s_Source_WingetCommunityDefault_Identifier;
            details.TrustLevel = SourceTrustLevel::Trusted | SourceTrustLevel::StoreOrigin;
            return details;
        }
        case WellKnownSource::MicrosoftStore:
        {
            SourceDetailsInternal details;
            details.Origin = SourceOrigin::Default;
            details.Name = s_Source_MSStoreDefault_Name;
            details.Type = Rest::RestSourceFactory::Type();
            details.Arg = s_Source_MSStoreDefault_Arg;
            details.Identifier = s_Source_MSStoreDefault_Identifier;
            details.TrustLevel = SourceTrustLevel::Trusted;
            details.SupportInstalledSearchCorrelation = false;
            return details;
        }
        case WellKnownSource::DesktopFrameworks:
        {
            SourceDetailsInternal details;
            details.Origin = SourceOrigin::Default;
            details.Name = s_Source_DesktopFrameworks_Name;
            details.Type = Microsoft::PreIndexedPackageSourceFactory::Type();
            details.Arg = s_Source_DesktopFrameworks_Arg;
            details.Data = s_Source_DesktopFrameworks_Data;
            details.Identifier = s_Source_DesktopFrameworks_Identifier;
            details.TrustLevel = SourceTrustLevel::Trusted | SourceTrustLevel::StoreOrigin;
            details.IsVisible = false;
            return details;
        }
        }

        THROW_HR(E_UNEXPECTED);
    }

    SourceList::SourceList() : m_userSourcesStream(Stream::UserSources), m_metadataStream(Stream::SourcesMetadata)
    {
        OverwriteSourceList();
        OverwriteMetadata();
    }

    std::vector<std::reference_wrapper<SourceDetailsInternal>> SourceList::GetCurrentSourceRefs()
    {
        std::vector<std::reference_wrapper<SourceDetailsInternal>> result;

        for (auto& s : m_sourceList)
        {
            if (!ShouldBeHidden(s))
            {
                result.emplace_back(std::ref(s));
            }
            else
            {
                AICLI_LOG(Repo, Info, << "GetCurrentSourceRefs: Source named '" << s.Name << "' from origin " << ToString(s.Origin) << " is hidden and is dropped.");
            }
        }

        return result;
    }

    auto SourceList::FindSource(std::string_view name, bool includeHidden)
    {
        return std::find_if(m_sourceList.begin(), m_sourceList.end(),
            [name, includeHidden](const SourceDetailsInternal& sd)
            {
                return Utility::ICUCaseInsensitiveEquals(sd.Name, name) &&
                    (includeHidden || !ShouldBeHidden(sd));
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
        bool sourcesSet = false;

        for (size_t i = 0; !sourcesSet && i < 10; ++i)
        {
            auto itr = FindSource(details.Name, true);
            THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS,
                itr != m_sourceList.end() && itr->Origin != SourceOrigin::Metadata && !itr->IsTombstone);

            // Erase the source's entry if applicable
            if (itr != m_sourceList.end())
            {
                m_sourceList.erase(itr);
            }

            m_sourceList.emplace_back(details);

            sourcesSet = SetSourcesByOrigin(SourceOrigin::User, m_sourceList);

            if (!sourcesSet)
            {
                OverwriteSourceList();
                OverwriteMetadata();
            }
        }

        THROW_HR_IF_MSG(E_UNEXPECTED, !sourcesSet, "Too many attempts at SetSourcesByOrigin");

        SaveMetadataInternal(details);
    }

    void SourceList::RemoveSource(const SourceDetailsInternal& detailsRef)
    {
        // Copy the incoming details because we might destroy the referenced structure
        // when reloading the source details from settings.
        SourceDetailsInternal details = detailsRef;
        bool sourcesSet = false;

        for (size_t i = 0; !sourcesSet && i < 10; ++i)
        {
            switch (details.Origin)
            {
            case SourceOrigin::Default:
            {
                auto target = FindSource(details.Name, true);
                if (target == m_sourceList.end())
                {
                    THROW_HR_MSG(E_UNEXPECTED, "Default source not in SourceList");
                }

                if (!target->IsTombstone)
                {
                    SourceDetailsInternal tombstone;
                    tombstone.Name = details.Name;
                    tombstone.IsTombstone = true;
                    tombstone.Origin = SourceOrigin::User;
                    m_sourceList.emplace_back(std::move(tombstone));
                }
            }
                break;
            case SourceOrigin::User:
            {
                auto target = FindSource(details.Name);
                if (target == m_sourceList.end())
                {
                    // Assumed that an update to the sources removed it first
                    return;
                }

                m_sourceList.erase(target);
            }
                break;
            case SourceOrigin::GroupPolicy:
                // This should have already been blocked higher up.
                AICLI_LOG(Repo, Error, << "Attempting to remove Group Policy source: " << details.Name);
                THROW_HR(E_UNEXPECTED);
            default:
                THROW_HR(E_UNEXPECTED);
            }

            sourcesSet = SetSourcesByOrigin(SourceOrigin::User, m_sourceList);

            if (!sourcesSet)
            {
                OverwriteSourceList();
                OverwriteMetadata();
            }
        }

        THROW_HR_IF_MSG(E_UNEXPECTED, !sourcesSet, "Too many attempts at SetSourcesByOrigin");

        SaveMetadataInternal(details, true);
    }

    void SourceList::SaveMetadata(const SourceDetailsInternal& details)
    {
        SaveMetadataInternal(details);
    }

    bool SourceList::CheckSourceAgreements(std::string_view sourceName, std::string_view agreementsIdentifier, ImplicitAgreementFieldEnum agreementFields)
    {
        if (agreementFields == ImplicitAgreementFieldEnum::None && agreementsIdentifier.empty())
        {
            // No agreements to be accepted.
            return true;
        }

        auto detailsInternal = GetCurrentSource(sourceName);
        if (!detailsInternal)
        {
            // Source not found.
            return false;
        }

        return static_cast<int>(agreementFields) == detailsInternal->AcceptedAgreementFields &&
            agreementsIdentifier == detailsInternal->AcceptedAgreementsIdentifier;
    }

    void SourceList::SaveAcceptedSourceAgreements(std::string_view sourceName, std::string_view agreementsIdentifier, ImplicitAgreementFieldEnum agreementFields)
    {
        if (agreementFields == ImplicitAgreementFieldEnum::None && agreementsIdentifier.empty())
        {
            // No agreements to be accepted.
            return;
        }

        auto detailsInternal = GetCurrentSource(sourceName);
        if (!detailsInternal)
        {
            // No source to update.
            return;
        }

        detailsInternal->AcceptedAgreementFields = static_cast<int>(agreementFields);
        detailsInternal->AcceptedAgreementsIdentifier = agreementsIdentifier;

        SaveMetadataInternal(*detailsInternal);
    }

    void SourceList::RemoveSettingsStreams()
    {
        Stream{ Stream::UserSources }.Remove();
        Stream{ Stream::SourcesMetadata }.Remove();
    }

    void SourceList::OverwriteSourceList()
    {
        m_sourceList.clear();

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
    }

    void SourceList::OverwriteMetadata()
    {
        auto metadata = GetMetadata();
        for (auto& metaSource : metadata)
        {
            auto source = GetSource(metaSource.Name);
            if (source)
            {
                metaSource.CopyMetadataFieldsTo(*source);
            }
            else
            {
                m_sourceList.emplace_back(std::move(metaSource));
            }
        }
    }

    // Gets the sources from a particular origin.
    std::vector<SourceDetailsInternal> SourceList::GetSourcesByOrigin(SourceOrigin origin)
    {
        std::vector<SourceDetailsInternal> result;

        switch (origin)
        {
        case SourceOrigin::Default:
        {
            if (IsWellKnownSourceEnabled(WellKnownSource::MicrosoftStore))
            {
                result.emplace_back(GetWellKnownSourceDetailsInternal(WellKnownSource::MicrosoftStore));
            }

            if (IsWellKnownSourceEnabled(WellKnownSource::WinGet))
            {
                result.emplace_back(GetWellKnownSourceDetailsInternal(WellKnownSource::WinGet));
            }

            // Since the source is not visible outside, this is added just to have the source in the internal
            // list for tracking updates.  Thus there is no need to check a policy.
            result.emplace_back(GetWellKnownSourceDetailsInternal(WellKnownSource::DesktopFrameworks));
        }
        break;
        case SourceOrigin::User:
        {
            std::vector<SourceDetailsInternal> userSources = GetSourcesFromSetting(
                m_userSourcesStream,
                s_SourcesYaml_Sources,
                [&](SourceDetailsInternal& details, const std::string& settingValue, const YAML::Node& source)
                {
                    std::string_view name = m_userSourcesStream.GetName();
                    if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Name, details.Name)) { return false; }
                    if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Type, details.Type)) { return false; }
                    if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Arg, details.Arg)) { return false; }
                    if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Data, details.Data)) { return false; }
                    if (!TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_IsTombstone, details.IsTombstone)) { return false; }
                    TryReadScalar(name, settingValue, source, s_SourcesYaml_Source_Identifier, details.Identifier, false);
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

    bool SourceList::SetSourcesByOrigin(SourceOrigin origin, const std::vector<SourceDetailsInternal>& sources)
    {
        switch (origin)
        {
        case SourceOrigin::User:
            return SetSourcesToSettingWithFilter(m_userSourcesStream, SourceOrigin::User, sources);
        }

        THROW_HR(E_UNEXPECTED);
    }

    std::vector<SourceDetailsInternal> SourceList::GetMetadata()
    {
        return GetSourcesFromSetting(
            m_metadataStream,
            s_MetadataYaml_Sources,
            [&](SourceDetailsInternal& details, const std::string& settingValue, const YAML::Node& source)
            {
                details.Origin = SourceOrigin::Metadata;
                std::string_view name = m_metadataStream.GetName();
                if (!TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_Name, details.Name)) { return false; }
                int64_t lastUpdateInEpoch{};
                if (!TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_LastUpdate, lastUpdateInEpoch)) { return false; }
                details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(lastUpdateInEpoch);
                TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_AcceptedAgreementsIdentifier, details.AcceptedAgreementsIdentifier, false);
                TryReadScalar(name, settingValue, source, s_MetadataYaml_Source_AcceptedAgreementFields, details.AcceptedAgreementFields, false);
                return true;
            });
    }

    bool SourceList::SetMetadata(const std::vector<SourceDetailsInternal>& sources)
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
            out << YAML::Key << s_MetadataYaml_Source_AcceptedAgreementsIdentifier << YAML::Value << details.AcceptedAgreementsIdentifier;
            out << YAML::Key << s_MetadataYaml_Source_AcceptedAgreementFields << YAML::Value << details.AcceptedAgreementFields;
            out << YAML::EndMap;
        }

        out << YAML::EndSeq;
        out << YAML::EndMap;

        return m_metadataStream.Set(out.str());
    }

    void SourceList::SaveMetadataInternal(const SourceDetailsInternal& detailsRef, bool remove)
    {
        // Copy the incoming details because we might overwrite the metadata
        // when reloading the source details from settings.
        SourceDetailsInternal details = detailsRef;
        bool metadataSet = false;

        for (size_t i = 0; !metadataSet && i < 10; ++i)
        {
            metadataSet = SetMetadata(m_sourceList);

            if (!metadataSet)
            {
                OverwriteMetadata();

                auto target = FindSource(details.Name, true);
                if (target == m_sourceList.end())
                {
                    // Didn't find the metadata, so we consider this a success
                    return;
                }

                if (remove)
                {
                    // The remove will have removed the source but not the metadata.
                    // Remove it again here.
                    m_sourceList.erase(target);
                }
                else
                {
                    // Update the freshly read metadata with the update that was requested.
                    details.CopyMetadataFieldsTo(*target);
                }
            }
        }

        THROW_HR_IF_MSG(E_UNEXPECTED, !metadataSet, "Too many attempts at SetMetadata");
    }
}
