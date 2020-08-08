// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRepositorySource.h"

#include "AggregatedSource.h"
#include "SourceFactory.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"

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
    constexpr std::string_view s_SourcesYaml_Source_IsTombstone = "IsTombstone"sv;

    constexpr std::string_view s_MetadataYaml_Sources = "Sources"sv;
    constexpr std::string_view s_MetadataYaml_Source_Name = "Name"sv;
    constexpr std::string_view s_MetadataYaml_Source_LastUpdate = "LastUpdate"sv;

    constexpr std::string_view s_Source_WingetCommunityDefault_Name = "winget"sv;
    constexpr std::string_view s_Source_WingetCommunityDefault_Arg = "https://winget.azureedge.net/cache"sv;
    constexpr std::string_view s_Source_WingetCommunityDefault_Data = "Microsoft.Winget.Source_8wekyb3d8bbwe"sv;

    namespace
    {
        // SourceDetails with additional data used by this file.
        struct SourceDetailsInternal : public SourceDetails
        {
            // If true, this is a tombstone, marking the deletion of a source at a lower priority origin.
            bool IsTombstone = false;
        };

        // Finds a source from the given vector by its name.
        auto FindSourceByName(std::vector<SourceDetailsInternal>& sources, std::string_view name)
        {
            return std::find_if(sources.begin(), sources.end(), [&name](const SourceDetailsInternal& sd) { return Utility::CaseInsensitiveEquals(sd.Name, name); });
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
            catch (const std::runtime_error& e)
            {
                AICLI_LOG(YAML, Error, << "Setting '" << settingName << "' contained invalid YAML (" << e.what() << "):\n" << settingValue);
                return false;
            }

            try
            {
                YAML::Node sources = document[std::string{ rootName }];
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

                for (const auto& source : sources)
                {
                    SourceDetailsInternal details;
                    if (!parse(details, settingValue, source))
                    {
                        return false;
                    }

                    result.emplace_back(std::move(details));
                }
            }
            catch (const std::runtime_error& e)
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

        // Gets the sources from a particular origin.
        std::vector<SourceDetailsInternal> GetSourcesByOrigin(SourceOrigin origin)
        {
            std::vector<SourceDetailsInternal> result;

            switch (origin)
            {
            case SourceOrigin::Default:
            {
                SourceDetailsInternal details;
                details.Name = s_Source_WingetCommunityDefault_Name;
                details.Type = Microsoft::PreIndexedPackageSourceFactory::Type();
                details.Arg = s_Source_WingetCommunityDefault_Arg;
                details.Data = s_Source_WingetCommunityDefault_Data;
                result.emplace_back(std::move(details));
            }
                break;
            case SourceOrigin::User:
                result = GetSourcesFromSetting(
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
                        return true;
                    });
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

        // Gets the internal view of the sources.
        std::vector<SourceDetailsInternal> GetSourcesInternal()
        {
            std::vector<SourceDetailsInternal> result;

            for (SourceOrigin origin : { SourceOrigin::User, SourceOrigin::Default })
            {
                auto forOrigin = GetSourcesByOrigin(origin);

                for (auto&& source : forOrigin)
                {
                    auto itr = FindSourceByName(result, source.Name);
                    if (itr == result.end())
                    {
                        // Name not already defined, add it
                        result.emplace_back(std::move(source));
                    }
                    else
                    {
                        AICLI_LOG(Repo, Info, << "Source named '" << itr->Name << "' is already defined at origin " << ToString(itr->Origin) <<
                            ". The source from origin " << ToString(origin) << " is dropped.");
                    }
                }
            }

            // Remove all tombstones, walking backwards.
            for (size_t j = result.size(); j > 0; --j)
            {
                size_t i = j - 1;

                if (result[i].IsTombstone)
                {
                    AICLI_LOG(Repo, Info, << "Source named '" << result[i].Name << "' from origin " << ToString(result[i].Origin) << " is a tombstone and is dropped.");
                    result.erase(result.begin() + i);
                }
            }

            auto metadata = GetMetadata();
            for (const auto& metaSource : metadata)
            {
                auto itr = FindSourceByName(result, metaSource.Name);
                if (itr != result.end())
                {
                    itr->LastUpdateTime = metaSource.LastUpdateTime;
                }
            }

            return result;
        }

        // Make up for the lack of string_view support in YAML CPP.
        YAML::Emitter& operator<<(YAML::Emitter& out, std::string_view sv)
        {
            return (out << std::string(sv));
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
                    out << YAML::Key << s_SourcesYaml_Source_IsTombstone << YAML::Value << details.IsTombstone;
                    out << YAML::EndMap;
                }
            }

            out << YAML::EndSeq;
            out << YAML::EndMap;

            Settings::SetSetting(setting, out.c_str());
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

            Settings::SetSetting(Settings::Streams::SourcesMetadata, out.c_str());
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
    }

    std::string_view ToString(SourceOrigin origin)
    {
        switch (origin)
        {
        case SourceOrigin::Default:
            return "Default"sv;
        case SourceOrigin::User:
            return "User"sv;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::vector<SourceDetails> GetSources()
    {
        auto internalResult = GetSourcesInternal();

        std::vector<SourceDetails> result;
        for (auto&& source : internalResult)
        {
            result.emplace_back(std::move(source));
        }

        return result;
    }

    std::optional<SourceDetails> GetSource(std::string_view name)
    {
        // Check all sources for the given name.
        auto currentSources = GetSourcesInternal();

        auto itr = FindSourceByName(currentSources, name);
        if (itr == currentSources.end())
        {
            return {};
        }
        else
        {
            return *itr;
        }
    }

    void AddSource(std::string_view name, std::string_view type, std::string_view arg, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        AICLI_LOG(Repo, Info, << "Adding source: Name[" << name << "], Type[" << type << "], Arg[" << arg << "]");

        // Check all sources for the given name.
        auto currentSources = GetSourcesInternal();

        auto itr = FindSourceByName(currentSources, name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, itr != currentSources.end());

        SourceDetailsInternal details;
        details.Name = name;
        details.Type = type;
        details.Arg = arg;
        details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(0);
        details.Origin = SourceOrigin::User;

        AddSourceFromDetails(details, progress);

        AICLI_LOG(Repo, Info, << "Source created with extra data: " << details.Data);
        currentSources.emplace_back(details);

        SetSourcesByOrigin(SourceOrigin::User, currentSources);
    }

    std::shared_ptr<ISource> OpenSource(std::string_view name, IProgressCallback& progress)
    {
        auto currentSources = GetSourcesInternal();

        if (name.empty())
        {
            if (currentSources.empty())
            {
                AICLI_LOG(Repo, Info, << "Default source requested, but no sources configured");
                return {};
            }
            else if(currentSources.size() == 1)
            {
                AICLI_LOG(Repo, Info, << "Default source requested, only 1 source available, using the only source: " << currentSources[0].Name);
                return OpenSource(currentSources[0].Name, progress);
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Default source requested, multiple sources available, creating aggregated source.");
                auto aggregatedSource = std::make_shared<AggregatedSource>();

                bool sourceUpdated = false;
                for (auto& source : currentSources)
                {
                    AICLI_LOG(Repo, Info, << "Adding to aggregated source: " << source.Name);

                    if (ShouldUpdateBeforeOpen(source))
                    {
                        // TODO: Consider adding a context callback to indicate we are doing the same action
                        // to avoid the progress bar fill up multiple times.
                        UpdateSourceFromDetails(source, progress);
                        sourceUpdated = true;
                    }
                    aggregatedSource->AddSource(CreateSourceFromDetails(source, progress));
                }

                if (sourceUpdated)
                {
                    SetMetadata(currentSources);
                }

                return aggregatedSource;
            }
        }
        else
        {
            auto itr = FindSourceByName(currentSources, name);
            
            if (itr == currentSources.end())
            {
                AICLI_LOG(Repo, Info, << "Named source requested, but not found: " << name);
                return {};
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Named source requested, found: " << itr->Name);
                if (ShouldUpdateBeforeOpen(*itr))
                {
                    UpdateSourceFromDetails(*itr, progress);
                    SetMetadata(currentSources);
                }
                return CreateSourceFromDetails(*itr, progress);
            }
        }
    }

    bool UpdateSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        auto currentSources = GetSourcesInternal();
        auto itr = FindSourceByName(currentSources, name);

        if (itr == currentSources.end())
        {
            AICLI_LOG(Repo, Info, << "Named source to be updated, but not found: " << name);
            return false;
        }
        else
        {
            AICLI_LOG(Repo, Info, << "Named source to be updated, found: " << itr->Name);
            UpdateSourceFromDetails(*itr, progress);

            SetMetadata(currentSources);
            return true;
        }
    }

    bool RemoveSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        auto currentSources = GetSourcesInternal();
        auto itr = FindSourceByName(currentSources, name);

        if (itr == currentSources.end())
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, but not found: " << name);
            return false;
        }
        else
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, found: " << itr->Name << " [" << ToString(itr->Origin) << ']');
            RemoveSourceFromDetails(*itr, progress);

            switch (itr->Origin)
            {
            case SourceOrigin::Default:
            {
                SourceDetailsInternal tombstone;
                tombstone.Name = name;
                tombstone.IsTombstone = true;
                tombstone.Origin = SourceOrigin::User;
                currentSources.emplace_back(std::move(tombstone));
            }
                break;
            case SourceOrigin::User:
                currentSources.erase(itr);
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            // Add back tombstoned default sources, otherwise the info will be lost by SetSourcesByOrigin
            auto defaultSources = GetSourcesByOrigin(SourceOrigin::Default);
            for (const auto& defaultSource : defaultSources)
            {
                if (FindSourceByName(currentSources, defaultSource.Name) == currentSources.end())
                {
                    SourceDetailsInternal tombstone;
                    tombstone.Name = defaultSource.Name;
                    tombstone.IsTombstone = true;
                    tombstone.Origin = SourceOrigin::User;
                    currentSources.emplace_back(std::move(tombstone));
                }
            }

            SetSourcesByOrigin(SourceOrigin::User, currentSources);

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
            auto currentSources = GetSourcesInternal();
            auto itr = FindSourceByName(currentSources, name);

            if (itr == currentSources.end())
            {
                AICLI_LOG(Repo, Info, << "Named source to be dropped, but not found: " << name);
                return false;
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Named source to be dropped, found: " << itr->Name);

                currentSources.erase(itr);

                // Since this only writes the user setting, it can't actually drop non-user sources.
                // But since it also implicitly sets all metadata, it will drop the metadata and allow
                // somewhat of a clean slate.
                SetSourcesByOrigin(SourceOrigin::User, currentSources);

                return true;
            }
        }
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
            result << " Inclusions:" << ApplicationMatchFieldToString(include.Field) << "='" << include.Value << "'[" << MatchTypeToString(include.Type) << "]";
        }

        for (const auto& filter : Filters)
        {
            result << " Filter:" << ApplicationMatchFieldToString(filter.Field) << "='" << filter.Value << "'[" << MatchTypeToString(filter.Type) << "]";
        }

        if (MaximumResults)
        {
            result << " Limit:" << MaximumResults;
        }

        return result.str();
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
