// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <Public/AppInstallerRepositorySource.h>
#include <AppInstallerDateTime.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::Runtime;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

// Duplicating here because a change to these values in the product *REALLY* needs to be thought through.
using namespace std::string_literals;
using namespace std::string_view_literals;
constexpr std::string_view s_RepositorySettings_UserSources = "usersources"sv;

constexpr std::string_view s_SourcesYaml_Sources = "Sources"sv;
constexpr std::string_view s_SourcesYaml_Source_Name = "Name"sv;
constexpr std::string_view s_SourcesYaml_Source_Type = "Type"sv;
constexpr std::string_view s_SourcesYaml_Source_Arg = "Arg"sv;
constexpr std::string_view s_SourcesYaml_Source_Data = "Data"sv;
constexpr std::string_view s_SourcesYaml_Source_LastUpdate = "LastUpdate"sv;

constexpr std::string_view s_EmptySources = R"(
Sources:
)"sv;

constexpr std::string_view s_SingleSource = R"(
Sources:
  - Name: testName
    Type: testType
    Arg: testArg
    Data: testData
    LastUpdate: 0
)"sv;

constexpr std::string_view s_ThreeSources = R"(
Sources:
  - Name: testName
    Type: testType
    Arg: testArg
    Data: testData
    LastUpdate: 0
  - Name: testName2
    Type: testType2
    Arg: testArg2
    Data: testData2
    LastUpdate: 1
  - Name: testName3
    Type: testType3
    Arg: testArg3
    Data: testData3
    LastUpdate: 2
)"sv;

constexpr std::string_view s_SingleSource_MissingArg = R"(
Sources:
  - Name: testName
    Type: testType
    Data: testData
    LastUpdate: 0
)"sv;

TEST_CASE("RepoSources_UserSettingDoesNotExist", "[sources]")
{
    RemoveSetting(s_RepositorySettings_UserSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.empty());
}

TEST_CASE("RepoSources_EmptySourcesList", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_EmptySources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.empty());
}

TEST_CASE("RepoSources_SingleSource", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_SingleSource);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == "testName");
    REQUIRE(sources[0].Type == "testType");
    REQUIRE(sources[0].Arg == "testArg");
    REQUIRE(sources[0].Data == "testData");
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));
}

TEST_CASE("RepoSources_ThreeSources", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_ThreeSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 3);

    const char* suffix[3] = { "", "2", "3" };

    for (size_t i = 0; i < 3; ++i)
    {
        INFO("Source #" << i);
        REQUIRE(sources[i].Name == "testName"s + suffix[i]);
        REQUIRE(sources[i].Type == "testType"s + suffix[i]);
        REQUIRE(sources[i].Arg == "testArg"s + suffix[i]);
        REQUIRE(sources[i].Data == "testData"s + suffix[i]);
        REQUIRE(sources[i].LastUpdateTime == ConvertUnixEpochToSystemClock(i));
    }
}

TEST_CASE("RepoSources_InvalidYAML", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, "Name: Value : BAD");

    REQUIRE_THROWS_HR(GetSources(), APPINSTALLER_CLI_ERROR_SOURCES_INVALID);
}

TEST_CASE("RepoSources_MissingField", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_SingleSource_MissingArg);

    REQUIRE_THROWS_HR(GetSources(), APPINSTALLER_CLI_ERROR_SOURCES_INVALID);
}

TEST_CASE("RepoSources_AddSource", "[sources]")
{
    RemoveSetting(s_RepositorySettings_UserSources);

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";

    auto source = AddSource(name, type, arg);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == "");
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));
}

TEST_CASE("RepoSources_AddMultipleSources", "[sources]")
{
    RemoveSetting(s_RepositorySettings_UserSources);

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";

    auto source = AddSource(name, type, arg);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == "");
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));

    const char* suffix[2] = { "", "2" };

    source = AddSource(name + suffix[1], type + suffix[1], arg + suffix[1]);

    sources = GetSources();
    REQUIRE(sources.size() == 2);

    for (size_t i = 0; i < 2; ++i)
    {
        INFO("Source #" << i);
        REQUIRE(sources[i].Name == name + suffix[i]);
        REQUIRE(sources[i].Type == type + suffix[i]);
        REQUIRE(sources[i].Arg == arg + suffix[i]);
        REQUIRE(sources[i].Data == "");
        REQUIRE(sources[i].LastUpdateTime == ConvertUnixEpochToSystemClock(0));
    }
}
