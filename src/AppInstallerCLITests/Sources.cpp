// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestSource.h"

#include <AppInstallerRepositorySource.h>
#include <AppInstallerDateTime.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <AppInstallerErrors.h>
#include <winget/Settings.h>

using namespace AppInstaller;
using namespace AppInstaller::Runtime;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

// Duplicating here because a change to these values in the product *REALLY* needs to be thought through.
using namespace std::string_literals;
using namespace std::string_view_literals;

constexpr std::string_view s_SourcesYaml_Sources = "Sources"sv;
constexpr std::string_view s_SourcesYaml_Source_Name = "Name"sv;
constexpr std::string_view s_SourcesYaml_Source_Type = "Type"sv;
constexpr std::string_view s_SourcesYaml_Source_Arg = "Arg"sv;
constexpr std::string_view s_SourcesYaml_Source_Data = "Data"sv;
constexpr std::string_view s_SourcesYaml_Source_LastUpdate = "LastUpdate"sv;

constexpr std::string_view s_EmptySources = R"(
Sources:
)"sv;

constexpr std::string_view s_DefaultSourceTombstoned = R"(
Sources:
  - Name: winget
    Type: ""
    Arg: ""
    Data: ""
    IsTombstone: true
)"sv;

constexpr std::string_view s_SingleSource = R"(
Sources:
  - Name: testName
    Type: testType
    Arg: testArg
    Data: testData
    IsTombstone: false
)"sv;

constexpr std::string_view s_ThreeSources = R"(
Sources:
  - Name: testName
    Type: testType
    Arg: testArg
    Data: testData
    IsTombstone: false
  - Name: testName2
    Type: testType2
    Arg: testArg2
    Data: testData2
    IsTombstone: false
  - Name: testName3
    Type: testType3
    Arg: testArg3
    Data: testData3
    IsTombstone: false
  - Name: winget
    Type: ""
    Arg: ""
    Data: ""
    IsTombstone: true
)"sv;

constexpr std::string_view s_ThreeSourcesMetadata = R"(
Sources:
  - Name: testName
    LastUpdate: 0
  - Name: testName2
    LastUpdate: 1
  - Name: testName3
    LastUpdate: 2
)"sv;

constexpr std::string_view s_SingleSource_MissingArg = R"(
Sources:
  - Name: testName
    Type: testType
    Data: testData
    IsTombstone: false
)"sv;

constexpr std::string_view s_TwoSource_AggregateSourceTest = R"(
Sources:
  - Name: winget
    Type: testType
    Arg: testArg
    Data: testData
    IsTombstone: false
  - Name: msstore
    Type: testType
    Arg: testArg
    Data: testData
    IsTombstone: false
)"sv;

namespace
{
    // Helper to create a simple source.
    struct SourcesTestSource : public TestCommon::TestSource
    {
        SourcesTestSource() = default;
        SourcesTestSource(const SourceDetails& details)
        {
            Details = details;
        }

        static std::shared_ptr<ISource> Create(const SourceDetails& details)
        {
            // using return std::make_shared<TestSource>(details); will crash the x86 test during destruction.
            return std::shared_ptr<ISource>(new SourcesTestSource(details));
        }

        SearchResult Search(const SearchRequest&) const override
        {
            SearchResult result;
            PackageMatchFilter testMatchFilter1{ PackageMatchField::Id, MatchType::Exact, "test" };
            PackageMatchFilter testMatchFilter2{ PackageMatchField::Name, MatchType::Exact, "test" };
            PackageMatchFilter testMatchFilter3{ PackageMatchField::Id, MatchType::CaseInsensitive, "test" };
            result.Matches.emplace_back(std::shared_ptr<IPackage>(), testMatchFilter1);
            result.Matches.emplace_back(std::shared_ptr<IPackage>(), testMatchFilter2);
            result.Matches.emplace_back(std::shared_ptr<IPackage>(), testMatchFilter3);
            return result;
        }
    };

    // Helper that allows some lambdas to be wrapped into a source factory.
    struct TestSourceFactory : public ISourceFactory
    {
        using CreateFunctor = std::function<std::shared_ptr<ISource>(const SourceDetails&)>;
        using AddFunctor = std::function<void(SourceDetails&)>;
        using UpdateFunctor = std::function<void(const SourceDetails&)>;
        using RemoveFunctor = std::function<void(const SourceDetails&)>;

        TestSourceFactory() :
            m_Create(SourcesTestSource::Create), m_Add([](SourceDetails&) {}), m_Update([](const SourceDetails&) {}), m_Remove([](const SourceDetails&) {}) {}

        // ISourceFactory
        std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback&) override
        {
            return m_Create(details);
        }

        void Add(SourceDetails& details, IProgressCallback&) override
        {
            m_Add(details);
        }

        void Update(const SourceDetails& details, IProgressCallback&) override
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

        CreateFunctor m_Create;
        AddFunctor m_Add;
        UpdateFunctor m_Update;
        RemoveFunctor m_Remove;
    };
}


TEST_CASE("RepoSources_UserSettingDoesNotExist", "[sources]")
{
    RemoveSetting(Streams::UserSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);
    REQUIRE(sources[0].Origin == SourceOrigin::Default);
}

TEST_CASE("RepoSources_EmptySourcesList", "[sources]")
{
    SetSetting(Streams::UserSources, s_EmptySources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);
    REQUIRE(sources[0].Origin == SourceOrigin::Default);
}

TEST_CASE("RepoSources_DefaultSourceTombstoned", "[sources]")
{
    SetSetting(Streams::UserSources, s_DefaultSourceTombstoned);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.empty());
}

TEST_CASE("RepoSources_SingleSource", "[sources]")
{
    SetSetting(Streams::UserSources, s_SingleSource);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 2);

    REQUIRE(sources[0].Name == "testName");
    REQUIRE(sources[0].Type == "testType");
    REQUIRE(sources[0].Arg == "testArg");
    REQUIRE(sources[0].Data == "testData");
    REQUIRE(sources[0].Origin == SourceOrigin::User);
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));

    REQUIRE(sources[1].Origin == SourceOrigin::Default);
}

TEST_CASE("RepoSources_ThreeSources", "[sources]")
{
    SetSetting(Streams::UserSources, s_ThreeSources);
    SetSetting(Streams::SourcesMetadata, s_ThreeSourcesMetadata);

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
        REQUIRE(sources[i].Origin == SourceOrigin::User);
    }
}

TEST_CASE("RepoSources_InvalidYAML", "[sources]")
{
    SetSetting(Streams::UserSources, "Name: Value : BAD");

    REQUIRE_THROWS_HR(GetSources(), APPINSTALLER_CLI_ERROR_SOURCES_INVALID);
}

TEST_CASE("RepoSources_MissingField", "[sources]")
{
    SetSetting(Streams::UserSources, s_SingleSource_MissingArg);

    REQUIRE_THROWS_HR(GetSources(), APPINSTALLER_CLI_ERROR_SOURCES_INVALID);
}

TEST_CASE("RepoSources_AddSource", "[sources]")
{
    SetSetting(Streams::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    bool addCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_Add = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    REQUIRE(addCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 2);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
    REQUIRE(sources[0].Origin == SourceOrigin::User);

    REQUIRE(sources[1].Origin == SourceOrigin::Default);
}

TEST_CASE("RepoSources_AddMultipleSources", "[sources]")
{
    SetSetting(Streams::UserSources, s_EmptySources);

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    const char* suffix[2] = { "", "2" };

    TestSourceFactory factory1;
    factory1.m_Add = [&](SourceDetails& sd) { sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory1);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 2);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
    REQUIRE(sources[0].Origin == SourceOrigin::User);

    REQUIRE(sources[1].Origin == SourceOrigin::Default);

    TestSourceFactory factory2;
    factory2.m_Add = [&](SourceDetails& sd) { sd.Data = data + suffix[1]; };
    TestHook_SetSourceFactoryOverride(type + suffix[1], factory2);

    AddSource(name + suffix[1], type + suffix[1], arg + suffix[1], progress);

    sources = GetSources();
    REQUIRE(sources.size() == 3);

    for (size_t i = 0; i < 2; ++i)
    {
        INFO("Source #" << i);
        REQUIRE(sources[i].Name == name + suffix[i]);
        REQUIRE(sources[i].Type == type + suffix[i]);
        REQUIRE(sources[i].Arg == arg + suffix[i]);
        REQUIRE(sources[i].Data == data + suffix[i]);
        REQUIRE(sources[i].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
        REQUIRE(sources[i].Origin == SourceOrigin::User);
    }

    REQUIRE(sources[2].Origin == SourceOrigin::Default);
}

TEST_CASE("RepoSources_UpdateSource", "[sources]")
{
    using namespace std::chrono_literals;

    SetSetting(Streams::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    bool addCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_Add = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    REQUIRE(addCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 2);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
    REQUIRE(sources[0].Origin == SourceOrigin::User);

    REQUIRE(sources[1].Origin == SourceOrigin::Default);

    // Reset for a call to update
    bool updateCalledOnFactory = false;
    auto now = std::chrono::system_clock::now();
    factory.m_Update = [&](const SourceDetails&) { updateCalledOnFactory = true; };

    UpdateSource(name, progress);

    REQUIRE(updateCalledOnFactory);

    sources = GetSources();
    REQUIRE(sources.size() == 2);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE((now - sources[0].LastUpdateTime) < 1s);
}

TEST_CASE("RepoSources_UpdateSourceRetries", "[sources]")
{
    using namespace std::chrono_literals;

    SetSetting(Streams::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::string name = "thisIsTheName";
    std::string type = "thisIsTheType";
    std::string arg = "thisIsTheArg";
    std::string data = "thisIsTheData";

    TestSourceFactory factory;
    factory.m_Add = [&](SourceDetails& sd) { sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    // Reset for a call to update
    bool updateShouldThrow = false;
    bool updateCalledOnFactoryAgain = false;
    factory.m_Update = [&](const SourceDetails&)
    {
        if (updateShouldThrow)
        {
            updateShouldThrow = false;
            THROW_HR(E_ACCESSDENIED);
        }
        updateCalledOnFactoryAgain = true;
    };

    UpdateSource(name, progress);

    REQUIRE(updateCalledOnFactoryAgain);
}

TEST_CASE("RepoSources_RemoveSource", "[sources]")
{
    SetSetting(Streams::UserSources, s_EmptySources);
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
    REQUIRE(sources.size() == 2);

    RemoveSource(name, progress);

    REQUIRE(removeCalledOnFactory);

    sources = GetSources();
    REQUIRE(sources.size() == 1);
}

TEST_CASE("RepoSources_RemoveDefaultSource", "[sources]")
{
    SetSetting(Streams::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 1);
    REQUIRE(sources[0].Origin == SourceOrigin::Default);

    bool removeCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_Remove = [&](const SourceDetails&) { removeCalledOnFactory = true; };
    TestHook_SetSourceFactoryOverride(sources[0].Type, factory);

    ProgressCallback progress;

    RemoveSource(sources[0].Name, progress);

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
    std::string data = "testData";

    bool updateCalledOnFactory = false;
    TestSourceFactory factory;
    factory.m_Update = [&](const SourceDetails&) { updateCalledOnFactory = true; };
    TestHook_SetSourceFactoryOverride(type, factory);

    SetSetting(Streams::UserSources, s_SingleSource);

    ProgressCallback progress;
    auto source = OpenSource(name, progress).Source;

    REQUIRE(updateCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 2);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
}

TEST_CASE("RepoSources_DropSourceByName", "[sources]")
{
    SetSetting(Streams::UserSources, s_ThreeSources);
    SetSetting(Streams::SourcesMetadata, s_ThreeSourcesMetadata);

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
        REQUIRE(sources[i].Origin == SourceOrigin::User);
    }
}

TEST_CASE("RepoSources_DropAllSources", "[sources]")
{
    SetSetting(Streams::UserSources, s_ThreeSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 3);

    DropSource({});

    sources = GetSources();
    REQUIRE(sources.size() == 1);
    REQUIRE(sources[0].Origin == SourceOrigin::Default);
}

TEST_CASE("RepoSources_SearchAcrossMultipleSources", "[sources]")
{
    TestHook_ClearSourceFactoryOverrides();
    TestSourceFactory factory;
    TestHook_SetSourceFactoryOverride("testType", factory);

    SetSetting(Streams::UserSources, s_TwoSource_AggregateSourceTest);

    ProgressCallback progress;
    auto source = OpenSource("", progress).Source;

    SearchRequest request;
    auto result = source->Search(request);
    REQUIRE(result.Matches.size() == 6);
    REQUIRE_FALSE(result.Truncated);
    // matches are sorted in expected order
    REQUIRE((result.Matches[0].MatchCriteria.Type == MatchType::Exact && result.Matches[0].MatchCriteria.Field == PackageMatchField::Id));
    REQUIRE((result.Matches[1].MatchCriteria.Type == MatchType::Exact && result.Matches[1].MatchCriteria.Field == PackageMatchField::Id));
    REQUIRE((result.Matches[2].MatchCriteria.Type == MatchType::Exact && result.Matches[2].MatchCriteria.Field == PackageMatchField::Name));
    REQUIRE((result.Matches[3].MatchCriteria.Type == MatchType::Exact && result.Matches[3].MatchCriteria.Field == PackageMatchField::Name));
    REQUIRE((result.Matches[4].MatchCriteria.Type == MatchType::CaseInsensitive && result.Matches[4].MatchCriteria.Field == PackageMatchField::Id));
    REQUIRE((result.Matches[5].MatchCriteria.Type == MatchType::CaseInsensitive && result.Matches[5].MatchCriteria.Field == PackageMatchField::Id));

    // when truncate required
    request.MaximumResults = 3;
    result = source->Search(request);
    REQUIRE(result.Matches.size() == 3);
    REQUIRE(result.Truncated);
    // matches are sorted in expected order
    REQUIRE((result.Matches[0].MatchCriteria.Type == MatchType::Exact && result.Matches[0].MatchCriteria.Field == PackageMatchField::Id));
    REQUIRE((result.Matches[1].MatchCriteria.Type == MatchType::Exact && result.Matches[1].MatchCriteria.Field == PackageMatchField::Id));
    REQUIRE((result.Matches[2].MatchCriteria.Type == MatchType::Exact && result.Matches[2].MatchCriteria.Field == PackageMatchField::Name));
}