// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRepositorySource.h"


namespace AppInstaller::Repository
{
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
    }

    std::unique_ptr<ISource> AddSource(std::string name, std::string type, std::string arg)
    {
        THROW_HR_IF(E_INVALIDARG, name.empty());
        THROW_HR_IF(E_INVALIDARG, type.empty());

        // Check all sources for the given name.
        std::vector<SourceDetails> currentSources = GetSources();

        auto itr = std::find_if(currentSources.begin(), currentSources.end(), [&name](const SourceDetails& sd) { return Utility::CaseInsensitiveEquals(sd.Name, name); });
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS, itr != currentSources.end());

        SourceDetails details;
        details.Name = std::move(name);
        details.Type = std::move(type);
        details.Arg = std::move(arg);
        details.LastUpdateTime = Utility::ConvertUnixEpochToSystemClock(0);

        // TODO: Implement actual source creation, for now we just add to the user setting.
        // Ex.
        //std::unique_ptr<ISource> result = CreateSourceFromDetails(details);

        currentSources = GetSourcesFromSetting(s_RepositorySettings_UserSources);
        // NOTE: When implementing creation as above, insert the details that we then get out of the result.
        currentSources.push_back(details);

        SetSourcesToSetting(s_RepositorySettings_UserSources, currentSources);

        return {};
    }

    std::unique_ptr<ISource> OpenSource(std::string_view name)
    {
        UNREFERENCED_PARAMETER(name);
        return {};
    }

    std::vector<SourceDetails> GetSources()
    {
        return GetSourcesFromSetting(s_RepositorySettings_UserSources);
    }

    void RemoveSource(std::string_view name)
    {
        UNREFERENCED_PARAMETER(name);
    }
}
