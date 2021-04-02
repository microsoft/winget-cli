// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "TestSource.h"

#include <AppInstallerRepositorySource.h>
#include <AppInstallerDateTime.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <AppInstallerErrors.h>
#include <winget/Settings.h>

using namespace TestCommon;
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

constexpr std::string_view s_DefaultSourceAsUserSource = R"(
Sources:
  - Name: not-winget
    Type: Microsoft.PreIndexed.Package
    Arg: https://winget.azureedge.net/cache
    Data: Microsoft.Winget.Source_8wekyb3d8bbwe
    IsTombstone: false
)"sv;

constexpr std::string_view s_UserSourceNamedLikeDefault = R"(
Sources:
  - Name: winget
    Type: testType
    Arg: testArg
    Data: testData
    IsTombstone: false
)"sv;

namespace
{
    // Helper to create a simple source.
    struct SourcesTestSource : public TestSource
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
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = data; };
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

    TestSourceFactory factory1{ SourcesTestSource::Create };
    factory1.OnAdd = [&](SourceDetails& sd) { sd.Data = data; };
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

    TestSourceFactory factory2{ SourcesTestSource::Create };
    factory2.OnAdd = [&](SourceDetails& sd) { sd.Data = data + suffix[1]; };
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
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = data; };
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
    factory.OnUpdate = [&](const SourceDetails&) { updateCalledOnFactory = true; };

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

    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { sd.Data = data; };
    TestHook_SetSourceFactoryOverride(type, factory);

    ProgressCallback progress;
    AddSource(name, type, arg, progress);

    // Reset for a call to update
    bool updateShouldThrow = false;
    bool updateCalledOnFactoryAgain = false;
    factory.OnUpdate = [&](const SourceDetails&)
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
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnRemove = [&](const SourceDetails&) { removeCalledOnFactory = true; };
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
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnRemove = [&](const SourceDetails&) { removeCalledOnFactory = true; };
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
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnUpdate = [&](const SourceDetails&) { updateCalledOnFactory = true; };
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
    TestSourceFactory factory{ SourcesTestSource::Create };
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

TEST_CASE("RepoSources_GroupPolicy_DefaultSource", "[sources][groupPolicy]")
{
    WHEN("Default source is disabled")
    {
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::DefaultSource, PolicyState::Disabled);

        SECTION("Get source")
        {
            // Listing the sources should not return the default.
            SetSetting(Streams::UserSources, s_EmptySources);

            auto sources = GetSources();
            REQUIRE(sources.empty());
        }
        SECTION("Add default source")
        {
            // We should not be able to add the default source manually.
            SetSetting(Streams::UserSources, s_EmptySources);

            ProgressCallback progress;
            REQUIRE_POLICY_EXCEPTION(
                AddSource("winget", "Microsoft.PreIndexed.Package", "https://winget.azureedge.net/cache", progress),
                TogglePolicy::Policy::DefaultSource);
        }
        SECTION("Ignore default source from user")
        {
            // We should ignore any existing user source that is the same as the default.
            SetSetting(Streams::UserSources, s_DefaultSourceAsUserSource);

            auto sources = GetSources();
            REQUIRE(sources.empty());
        }
        SECTION("Add same-name source from user")
        {
            // We should allow adding sources with the same name as the default but
            // pointing somewhere else.
            SetSetting(Streams::UserSources, s_EmptySources);
            TestHook_ClearSourceFactoryOverrides();

            std::string name = "winget";
            std::string type = "someType";
            std::string arg = "notWingetRealArg";
            std::string data = "someData";

            bool addCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnAdd = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = data; };
            TestHook_SetSourceFactoryOverride(type, factory);

            ProgressCallback progress;
            AddSource(name, type, arg, progress);

            REQUIRE(addCalledOnFactory);

            auto sources = GetSources();
            REQUIRE(sources.size() == 1);

            REQUIRE(sources[0].Name == name);
            REQUIRE(sources[0].Type == type);
            REQUIRE(sources[0].Arg == arg);
            REQUIRE(sources[0].Data == data);
            REQUIRE(sources[0].Origin == SourceOrigin::User);
        }
        SECTION("Allow same name source from user")
        {
            // We should respect existing user sources with the same name.
            // We should allow adding sources with the same name as the default but
            // pointing somewhere else.
            SetSetting(Streams::UserSources, s_UserSourceNamedLikeDefault);

            auto sources = GetSources();
            REQUIRE(sources.size() == 1);

            REQUIRE(sources[0].Name == "winget");
            REQUIRE(sources[0].Type == "testType");
            REQUIRE(sources[0].Arg == "testArg");
            REQUIRE(sources[0].Data == "testData");
            REQUIRE(sources[0].Origin == SourceOrigin::User);
        }
    }

    WHEN("Default source is enabled")
    {
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::DefaultSource, PolicyState::Enabled);

        SECTION("Remove source is blocked")
        {
            // We should not be able to remove the default source.
            SetSetting(Streams::UserSources, s_EmptySources);

            ProgressCallback progress;
            REQUIRE_POLICY_EXCEPTION(
                RemoveSource("winget", progress),
                TogglePolicy::Policy::DefaultSource);
        }
        SECTION("Tombstone is overridden")
        {
            // We should ignore if the default source was already deleted.
            SetSetting(Streams::UserSources, s_DefaultSourceTombstoned);

            auto sources = GetSources();
            REQUIRE(sources.size() == 1);
            REQUIRE(sources[0].Name == "winget");
            REQUIRE(sources[0].Origin == SourceOrigin::Default);
        }
        SECTION("Same name source is overridden")
        {
            // We should ignore existing user sources with the same name as the default.
            SetSetting(Streams::UserSources, s_UserSourceNamedLikeDefault);

            auto sources = GetSources();
            REQUIRE(sources.size() == 1);

            REQUIRE(sources[0].Name == "winget");
            REQUIRE(sources[0].Arg == "https://winget.azureedge.net/cache");
            REQUIRE(sources[0].Origin == SourceOrigin::Default);
        }
    }
}

TEST_CASE("RepoSources_GroupPolicy_AdditionalSources", "[sources][groupPolicy]")
{
    WHEN("Additional sources are enabled")
    {
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::AdditionalSources, PolicyState::Enabled);

        SECTION("Additional sources are listed")
        {
            // Getting the current sources should list the additional sources.
            std::vector<SourceFromPolicy> policySources;
            const std::string suffix[3] = { "", "2", "3" };
            for (size_t i = 0; i < 3; ++i)
            {
                SourceFromPolicy source;
                source.Name = "name" + suffix[i];
                source.Type = "type" + suffix[i];
                source.Arg = "arg" + suffix[i];
                source.Data = "data" + suffix[i];
                source.Identifier = "id" + suffix[i];
                policySources.emplace_back(std::move(source));
            }

            policies.SetValue<ValuePolicy::AdditionalSources>(policySources);
            SetSetting(Streams::UserSources, s_EmptySources);

            auto sources = GetSources();

            // The source list includes the default source
            REQUIRE(sources.size() == policySources.size() + 1);
            REQUIRE(sources.back().Origin == SourceOrigin::Default);

            for (size_t i = 0; i < policySources.size(); ++i)
            {
                REQUIRE(sources[i].Name == policySources[i].Name);
                REQUIRE(sources[i].Type == policySources[i].Type);
                REQUIRE(sources[i].Arg == policySources[i].Arg);
                REQUIRE(sources[i].Data == policySources[i].Data);
                REQUIRE(sources[i].Identifier == policySources[i].Identifier);
                REQUIRE(sources[i].Origin == SourceOrigin::GroupPolicy);
            }
        }
        SECTION("Same-name user source is overridden")
        {
            // User sources with the same name as an additional source are ignored.
            SourceFromPolicy policySource;
            policySource.Name = "testName";
            policySource.Type = "notTestType";
            policySource.Arg = "notTestArg";
            policySource.Data = "notTestData";
            policySource.Identifier = "notTestId";

            policies.SetValue<ValuePolicy::AdditionalSources>({ policySource });
            SetSetting(Streams::UserSources, s_SingleSource);

            auto sources = GetSources();

            // The source list includes the default source
            REQUIRE(sources.size() == 2);
            REQUIRE(sources[1].Origin == SourceOrigin::Default);

            REQUIRE(sources[0].Name == policySource.Name);
            REQUIRE(sources[0].Type == policySource.Type);
            REQUIRE(sources[0].Arg == policySource.Arg);
            REQUIRE(sources[0].Data == policySource.Data);
            REQUIRE(sources[0].Identifier == policySource.Identifier);
            REQUIRE(sources[0].Origin == SourceOrigin::GroupPolicy);
        }
        SECTION("Cannot remove additional source")
        {
            // An additional source cannot be removed.
            SourceFromPolicy policySource;
            policySource.Name = "name";
            policySource.Type = "type";
            policySource.Arg = "arg";
            policySource.Data = "data";
            policySource.Identifier = "id";

            policies.SetValue<ValuePolicy::AdditionalSources>({ policySource });
            SetSetting(Streams::UserSources, s_EmptySources);

            ProgressCallback progress;
            REQUIRE_POLICY_EXCEPTION(
                RemoveSource(policySource.Name, progress),
                TogglePolicy::Policy::AdditionalSources);
        }
        SECTION("Additional source overrides default")
        {
            // An additional source with the same name as a default overrides it.
            SourceFromPolicy policySource;
            policySource.Name = "winget";
            policySource.Type = "notDefaultType";
            policySource.Arg = "notDefaultArg";
            policySource.Data = "notDefaultData";
            policySource.Identifier = "notDefaultId";

            policies.SetValue<ValuePolicy::AdditionalSources>({ policySource });
            SetSetting(Streams::UserSources, s_EmptySources);

            auto sources = GetSources();

            REQUIRE(sources.size() == 1);
            REQUIRE(sources[0].Name == policySource.Name);
            REQUIRE(sources[0].Type == policySource.Type);
            REQUIRE(sources[0].Arg == policySource.Arg);
            REQUIRE(sources[0].Data == policySource.Data);
            REQUIRE(sources[0].Identifier == policySource.Identifier);
            REQUIRE(sources[0].Origin == SourceOrigin::GroupPolicy);
        }
    }
}

TEST_CASE("RepoSources_GroupPolicy_AllowedSources", "[sources][groupPolicy]")
{
    WHEN("Allowed sources are enabled")
    {
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::AllowedSources, PolicyState::Enabled);

        SECTION("Add allowed source")
        {
            // We should be able to add sources in the allow list.
            SourceFromPolicy policySource;
            policySource.Name = "testName";
            policySource.Type = "testType";
            policySource.Arg = "testArg";
            policySource.Data = "testData";
            policySource.Identifier = "testId";

            policies.SetValue<ValuePolicy::AllowedSources>({ policySource });
            SetSetting(Streams::UserSources, s_EmptySources);
            TestHook_ClearSourceFactoryOverrides();

            bool addCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnAdd = [&](SourceDetails& sd)
            {
                addCalledOnFactory = true;
                sd.Data = policySource.Data;
                sd.Identifier = policySource.Identifier;
            };
            TestHook_SetSourceFactoryOverride(policySource.Type, factory);

            ProgressCallback progress;
            AddSource(policySource.Name, policySource.Type, policySource.Arg, progress);

            REQUIRE(addCalledOnFactory);

            // The source list includes the default source
            auto sources = GetSources();
            REQUIRE(sources.size() == 2);
            REQUIRE(sources[1].Origin == SourceOrigin::Default);

            REQUIRE(sources[0].Name == policySource.Name);
            REQUIRE(sources[0].Type == policySource.Type);
            REQUIRE(sources[0].Arg == policySource.Arg);
            REQUIRE(sources[0].Data == policySource.Data);
            REQUIRE(sources[0].Identifier == policySource.Identifier);
            REQUIRE(sources[0].Origin == SourceOrigin::User);
        }
        SECTION("Cannot add non-allowed source")
        {
            // We should not be allowed to add anything not matching the allow list.
            SourceFromPolicy policySource;
            policySource.Name = "testName";
            policySource.Type = "testType";
            policySource.Arg = "testArg";
            policySource.Data = "testData";
            policySource.Identifier = "testId";

            policies.SetValue<ValuePolicy::AllowedSources>({ policySource });
            SetSetting(Streams::UserSources, s_EmptySources);

            bool addCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnAdd = [&](SourceDetails&) { addCalledOnFactory = true; };

            ProgressCallback progress;
            REQUIRE_POLICY_EXCEPTION(
                AddSource("notAllowed", "type", "arg", progress),
                TogglePolicy::Policy::AllowedSources);
            REQUIRE_FALSE(addCalledOnFactory);
        }
    }

    WHEN("Allowed sources are disabled")
    {
        GroupPolicyTestOverride policies;
        policies.SetState(TogglePolicy::Policy::AllowedSources, PolicyState::Disabled);

        SECTION("Cannot add any source")
        {
            SetSetting(Streams::UserSources, s_EmptySources);

            bool addCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnAdd = [&](SourceDetails&) { addCalledOnFactory = true; };

            ProgressCallback progress;
            REQUIRE_POLICY_EXCEPTION(
                AddSource("name", "type", "arg", progress),
                TogglePolicy::Policy::AllowedSources);
            REQUIRE_FALSE(addCalledOnFactory);

            auto sources = GetSources();
            REQUIRE(sources.size() == 1);
            REQUIRE(sources[0].Origin == SourceOrigin::Default);
        }
        SECTION("Existing sources are ignored")
        {
            SetSetting(Streams::UserSources, s_SingleSource);

            auto sources = GetSources();
            REQUIRE(sources.size() == 1);
            REQUIRE(sources[0].Origin == SourceOrigin::Default);
        }
    }
}
