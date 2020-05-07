// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"

#include <AppInstallerRepositorySource.h>
#include <AppInstallerDateTime.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller;
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
    IsDefault: 1
  - Name: testName2
    Type: testType2
    Arg: testArg2
    Data: testData2
    LastUpdate: 1
    IsDefault: 0
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

// Helper to create a simple source.
struct TestSource : public ISource
{
    TestSource() = default;
    TestSource(const SourceDetails& details) : m_details(details) {}

    static std::shared_ptr<ISource> Create(const SourceDetails& details)
    {
        // using return std::make_shared<TestSource>(details); will crash the x86 test during destruction.
        return std::shared_ptr<ISource>(new TestSource(details));
    }

    // ISource
    const SourceDetails& GetDetails() const override
    {
        return m_details;
    }

    SearchResult Search(const SearchRequest& request) override
    {
        UNREFERENCED_PARAMETER(request);
        return {};
    }

    SourceDetails m_details;
};

// Helper that allows some lambdas to be wrapped into a source factory.
struct TestSourceFactory : public ISourceFactory
{
    using IsInitializedFunctor = std::function<bool(const SourceDetails&)>;
    using CreateFunctor = std::function<std::shared_ptr<ISource>(const SourceDetails&)>;
    using UpdateFunctor = std::function<void(SourceDetails&)>;
    using RemoveFunctor = std::function<void(const SourceDetails&)>;

    TestSourceFactory() :
        m_isInit([](const SourceDetails&) { return true; }), m_Create(TestSource::Create), m_Update([](SourceDetails&) {}), m_Remove([](const SourceDetails&) {}) {}

    // ISourceFactory
    bool IsInitialized(const SourceDetails& details) override
    {
        return m_isInit(details);
    }

    std::shared_ptr<ISource> Create(const SourceDetails& details) override
    {
        return m_Create(details);
    }

    void Update(SourceDetails& details, IProgressCallback&) override
    {
        m_Update(details);
    }

    void Remove(const SourceDetails& details, IProgressCallback&) override
    {
        m_Remove(details);
    }

    // Make copies of self when requested.
    operator std::function<std::unique_ptr<ISourceFactory>()>()
    {
        return [this]() { return std::make_unique<TestSourceFactory>(*this); };
    }

    IsInitializedFunctor m_isInit;
    CreateFunctor m_Create;
    UpdateFunctor m_Update;
    RemoveFunctor m_Remove;
};


TEST_CASE("RepoSources_UserSettingDoesNotExist", "[sources]")
{
    RemoveSetting(s_RepositorySettings_UserSources);

    std::vector<SourceDetails> sources = GetSources();
    // The default source is added when no source exists
    REQUIRE(sources.size() == 1);
    REQUIRE(sources[0].IsDefault);
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
        REQUIRE(sources[i].IsDefault == (i == 0));
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
    SetSetting(s_RepositorySettings_UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    bool updateCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_Update = [&](SourceDetails& sd) { updateCalledOnFactory = true; sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    REQUIRE(updateCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));
}

TEST_CASE("RepoSources_AddMultipleSources", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_EmptySources);

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    const char* suffix[2] = { "", "2" };

    TestSourceFactory factory1;
    factory1.m_Update = [&](SourceDetails& sd) { sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory1);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));

    TestSourceFactory factory2;
    factory2.m_Update = [&](SourceDetails& sd) { sd.Data = data + suffix[1]; };
    TestHook_SetSourceFactoryOverride(type + suffix[1], factory2);

    AddSource(name + suffix[1], type + suffix[1], arg + suffix[1], progress);

    sources = GetSources();
    REQUIRE(sources.size() == 2);

    for (size_t i = 0; i < 2; ++i)
    {
        INFO("Source #" << i);
        REQUIRE(sources[i].Name == name + suffix[i]);
        REQUIRE(sources[i].Type == type + suffix[i]);
        REQUIRE(sources[i].Arg == arg + suffix[i]);
        REQUIRE(sources[i].Data == data + suffix[i]);
        REQUIRE(sources[i].LastUpdateTime == ConvertUnixEpochToSystemClock(0));
    }
}

TEST_CASE("RepoSources_UpdateSource", "[sources]")
{
    using namespace std::chrono_literals;

    SetSetting(s_RepositorySettings_UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    bool updateCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_Update = [&](SourceDetails& sd) { updateCalledOnFactory = true; sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    REQUIRE(updateCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));

    // Reset for a call to update
    updateCalledOnFactory = false;
    auto now = std::chrono::system_clock::now();
    factory.m_Update = [&](SourceDetails& sd) { updateCalledOnFactory = true; sd.LastUpdateTime = now; };

    UpdateSource(name, progress);

    REQUIRE(updateCalledOnFactory);

    sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE((now - sources[0].LastUpdateTime) < 1s);
}

TEST_CASE("RepoSources_UpdateSourceRetries", "[sources]")
{
    using namespace std::chrono_literals;

    SetSetting(s_RepositorySettings_UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    TestSourceFactory factory;
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    // Reset for a call to update
    bool updateShouldThrow = false;
    bool updateCalledOnFactoryAgain = false;
    factory.m_Update = [&](SourceDetails& sd)
    {
        if (updateShouldThrow)
        {
            updateShouldThrow = false;
            THROW_HR(E_ACCESSDENIED);
        }
        updateCalledOnFactoryAgain = true;
        sd.Data = data;
    };

    UpdateSource(name, progress);

    REQUIRE(updateCalledOnFactoryAgain);
}

TEST_CASE("RepoSources_RemoveSource", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    bool removeCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_Remove = [&](const SourceDetails&) { removeCalledOnFactory = true; };
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    RemoveSource(name, progress);

    REQUIRE(removeCalledOnFactory);

    sources = GetSources();
    REQUIRE(sources.empty());
}

TEST_CASE("RepoSources_UpdateOnOpen", "[sources]")
{
    using namespace std::chrono_literals;

    TestHook_ClearSourceFactoryOverrides();

    std::string name = "testName";
    std::string type = "testType";
    std::string arg = "testArg";
    std::string data = "testDataOnUpdate";

    bool updateCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_isInit = [](const SourceDetails&) { return false; };
    factory.m_Update = [&](SourceDetails& sd) { updateCalledOnFactory = true; sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory);

    SetSetting(s_RepositorySettings_UserSources, s_SingleSource);

    ProgressCallback progress;
    auto source = OpenSource(name, progress);

    REQUIRE(updateCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));
}

TEST_CASE("RepoSources_DropSourceByName", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_ThreeSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 3);

    DropSource("testName");

    sources = GetSources();
    REQUIRE(sources.size() == 2);

    const char* suffix[2] = { "2", "3" };

    for (size_t i = 0; i < 2; ++i)
    {
        INFO("Source #" << i);
        REQUIRE(sources[i].Name == "testName"s + suffix[i]);
        REQUIRE(sources[i].Type == "testType"s + suffix[i]);
        REQUIRE(sources[i].Arg == "testArg"s + suffix[i]);
        REQUIRE(sources[i].Data == "testData"s + suffix[i]);
        REQUIRE(sources[i].LastUpdateTime == ConvertUnixEpochToSystemClock(i + 1));
        REQUIRE(!sources[i].IsDefault);
    }
}

TEST_CASE("RepoSources_DropAllSources", "[sources]")
{
    SetSetting(s_RepositorySettings_UserSources, s_ThreeSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 3);

    DropSource({});

    sources = GetSources();
    REQUIRE(sources.size() == 1);
    REQUIRE(sources[0].IsDefault);
}
