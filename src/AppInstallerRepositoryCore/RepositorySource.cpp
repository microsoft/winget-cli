// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRepositorySource.h"

#include "SourceFactory.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"

namespace AppInstaller::Repository
{
    using namespace std::chrono_literals;
    using namespace std::string_view_literals;
    constexpr std::string_view s_RepositorySettings_UserSources = "usersources"sv;

    constexpr std::string_view s_SourcesYaml_Sources = "Sources"sv;
    constexpr std::string_view s_SourcesYaml_Source_Name = "Name"sv;
    constexpr std::string_view s_SourcesYaml_Source_Type = "Type"sv;
    constexpr std::string_view s_SourcesYaml_Source_Arg = "Arg"sv;
    constexpr std::string_view s_SourcesYaml_Source_Data = "Data"sv;
    constexpr std::string_view s_SourcesYaml_Source_LastUpdate = "LastUpdate"sv;

    namespace
    {
        // Attempts to read a single scalar value from the node.
        template<typename Value>
        bool TryReadScalar(std::string_view settingName, const std::string& settingValue, const YAML::Node& sourceNode, std::string_view name, Value& value)
        {
            YAML::Node valueNode = sourceNode[std::string{ name }];

            if (!valueNode || !valueNode.IsScalar())
            {
                AICLI_LOG(Repo, Error, << "Setting '" << settingName << "' did not contain the expected format (" << name << " is invalid within a source):\n" << settingValue);
                return false;
            }

            value = valueNode.as<Value>();
            return true;
        }

        // Attempts to read the source details from the given stream.
        // Results are all or nothing; if any failures occur, no details are returned.
        bool TryReadSourceDetails(std::string_view settingName, std::istream& stream, std::vector<SourceDetails>& sourceDetails)
        {
            sourceDetails.clear();

            std::vector<SourceDetails> result;
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
                YAML::Node sources = document[std::string{ s_SourcesYaml_Sources }];
                if (!sources)
                {
                    AICLI_LOG(Repo, Error, << "Setting '" << settingName << "' did not contain the expected format (missing " << s_SourcesYaml_Sources << "):\n" << settingValue);
                    return false;
                }

                if (sources.IsNull())
                {
                    // An empty sources is an acceptable thing.
                    return true;
                }

                if (!sources.IsSequence())
                {
                    AICLI_LOG(Repo, Error, << "Setting '" << settingName << "' did not contain the expected format (" << s_SourcesYaml_Sources << " was not a sequence):\n" << settingValue);
                    return false;
                }

                for (const auto& source : sources)
                {
                    SourceDetails details;
                    if (!TryReadScalar(settingName, settingValue, source, s_SourcesYaml_Source_Name, details.Name)) { return false; }
                    if (!TryReadScalar(settingName, settingValue, source, s_SourcesYaml_Source_Type, details.Type)) { return false; }
                    if (!TryReadScalar(settingName, settingValue, source, s_SourcesYaml_Source_Arg, details.Arg)) { return false; }
                    if (!TryReadScalar(settingName, settingValue, source, s_SourcesYaml_Source_Data, details.Data)) { return false; }
                    int64_t lastUpdateInEpoch{};
                    if (!TryReadScalar(settingName, settingValue, source, s_SourcesYaml_Source_LastUpdate, lastUpdateInEpoch)) { return false; }
                    details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(lastUpdateInEpoch);

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

        // Gets the source details from a particular setting.
        std::vector<SourceDetails> GetSourcesFromSetting(std::string_view settingName)
        {
            auto sourcesStream = Runtime::GetSettingStream(settingName);
            if (!sourcesStream)
            {
                // TODO: Handle first run scenario and configure default source(s).
                //       Note that this case is different than the one in which all sources have been removed.
                return {};
            }
            else
            {
                std::vector<SourceDetails> result;
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCES_INVALID, !TryReadSourceDetails(settingName, *sourcesStream, result));
                return result;
            }
        }

        // Make up for the lack of string_view support in YAML CPP.
        YAML::Emitter& operator<<(YAML::Emitter& out, std::string_view sv)
        {
            return (out << std::string(sv));
        }

        // Sets the sources for a particular setting.
        void SetSourcesToSetting(std::string_view settingName, const std::vector<SourceDetails>& sources)
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << s_SourcesYaml_Sources;
            out << YAML::BeginSeq;

            for (const SourceDetails& details : sources)
            {
                out << YAML::BeginMap;
                out << YAML::Key << s_SourcesYaml_Source_Name << YAML::Value << details.Name;
                out << YAML::Key << s_SourcesYaml_Source_Type << YAML::Value << details.Type;
                out << YAML::Key << s_SourcesYaml_Source_Arg << YAML::Value << details.Arg;
                out << YAML::Key << s_SourcesYaml_Source_Data << YAML::Value << details.Data;
                out << YAML::Key << s_SourcesYaml_Source_LastUpdate << YAML::Value << Utility::ConvertSystemClockToUnixEpoch(details.LastUpdateTime);
                out << YAML::EndMap;
            }

            out << YAML::EndSeq;
            out << YAML::EndMap;

            Runtime::SetSetting(settingName, out.c_str());
        }

        // Finds a source from the given vector by its name.
        auto FindSourceByName(std::vector<SourceDetails>& sources, std::string_view name)
        {
            return std::find_if(sources.begin(), sources.end(), [&name](const SourceDetails& sd) { return Utility::CaseInsensitiveEquals(sd.Name, name); });
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

        bool CheckIfInitializedFromDetails(const SourceDetails& details)
        {
            return GetFactoryForType(details.Type)->IsInitialized(details);
        }

        std::shared_ptr<ISource> CreateSourceFromDetails(const SourceDetails& details)
        {
            return GetFactoryForType(details.Type)->Create(details);
        }

        void UpdateSourceFromDetails(SourceDetails& details, IProgressCallback& progress)
        {
            auto factory = GetFactoryForType(details.Type);

            // Attempt to update; if it fails, wait a short time and retry.
            try
            {
                factory->Update(details, progress);
                return;
            }
            CATCH_LOG();

            AICLI_LOG(Repo, Info, << "Source update failed, waiting a bit and retrying: " << details.Name);
            std::this_thread::sleep_for(2s);

            // If this one fails, maybe the problem is persistent.
            factory->Update(details, progress);
        }

        void RemoveSourceFromDetails(const SourceDetails& details, IProgressCallback& progress)
        {
            auto factory = GetFactoryForType(details.Type);

            if (factory->IsInitialized(details))
            {
                factory->Remove(details, progress);
            }
            else
            {
                AICLI_LOG(Repo, Info, << "Uninitialized source being removed, making it a no-op: " << details.Name);
            }
        }

        // Determines whether (and logs why) a source should be updated before it is opened.
        bool ShouldUpdateBeforeOpen(const SourceDetails& details)
        {
            if (!CheckIfInitializedFromDetails(details))
            {
                AICLI_LOG(Repo, Info, << "Source needs to be initialized during open: " << details.Name);
                return true;
            }

            // TODO: Enable some amount of user control over this.
            constexpr static auto s_DefaultAutoUpdateTime = 5min;

            auto timeSinceLastUpdate = std::chrono::system_clock::now() - details.LastUpdateTime;
            if (timeSinceLastUpdate > s_DefaultAutoUpdateTime)
            {
                AICLI_LOG(Repo, Info, << "Source past auto update time [" << 
                    std::chrono::duration_cast<std::chrono::minutes>(s_DefaultAutoUpdateTime).count() << " mins]; it has been at least " << 
                    std::chrono::duration_cast<std::chrono::minutes>(timeSinceLastUpdate).count() << " mins");
                return true;
            }

            return false;
        }
    }

    std::vector<SourceDetails> GetSources()
    {
        return GetSourcesFromSetting(s_RepositorySettings_UserSources);
    }

    std::optional<SourceDetails> GetSource(std::string_view name)
    {
        // Check all sources for the given name.
        std::vector<SourceDetails> currentSources = GetSources();

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

    void AddSource(std::string name, std::string type, std::string arg, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        AICLI_LOG(Repo, Info, << "Adding source: Name[" << name << "], Type[" << type << "], Arg[" << arg << "]");

        // Check all sources for the given name.
        std::vector<SourceDetails> currentSources = GetSources();

        auto itr = FindSourceByName(currentSources, name);
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, itr != currentSources.end());

        SourceDetails details;
        details.Name = std::move(name);
        details.Type = std::move(type);
        details.Arg = std::move(arg);
        details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(0);

        UpdateSourceFromDetails(details, progress);

        AICLI_LOG(Repo, Info, << "Source created with extra data: " << details.Data);

        currentSources = GetSourcesFromSetting(s_RepositorySettings_UserSources);
        currentSources.emplace_back(details);

        SetSourcesToSetting(s_RepositorySettings_UserSources, currentSources);
    }

    std::shared_ptr<ISource> OpenSource(std::string_view name, IProgressCallback& progress)
    {
        std::vector<SourceDetails> currentSources = GetSources();

        if (name.empty())
        {
            if (currentSources.empty())
            {
                AICLI_LOG(Repo, Info, << "Default source requested, but no sources configured");
                return {};
            }
            else
            {
                // TODO: Create aggregate source here.  For now, just get the first in the list.
                AICLI_LOG(Repo, Info, << "Default source requested, using first source: " << currentSources[0].Name);
                return OpenSource(currentSources[0].Name, progress);
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
                    SetSourcesToSetting(s_RepositorySettings_UserSources, currentSources);
                }
                return CreateSourceFromDetails(*itr);
            }
        }
    }

    bool UpdateSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        std::vector<SourceDetails> currentSources = GetSourcesFromSetting(s_RepositorySettings_UserSources);
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

            SetSourcesToSetting(s_RepositorySettings_UserSources, currentSources);
            return true;
        }
    }

    bool RemoveSource(std::string_view name, IProgressCallback& progress)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());

        std::vector<SourceDetails> currentSources = GetSourcesFromSetting(s_RepositorySettings_UserSources);
        auto itr = FindSourceByName(currentSources, name);

        if (itr == currentSources.end())
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, but not found: " << name);
            return false;
        }
        else
        {
            AICLI_LOG(Repo, Info, << "Named source to be removed, found: " << itr->Name);
            RemoveSourceFromDetails(*itr, progress);

            currentSources.erase(itr);
            SetSourcesToSetting(s_RepositorySettings_UserSources, currentSources);

            return true;
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
