// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "TestSource.h"

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

constexpr size_t c_DefaultSourceCount = 2;

constexpr std::string_view s_SourcesYaml_Sources = "Sources"sv;
constexpr std::string_view s_SourcesYaml_Source_Name = "Name"sv;
constexpr std::string_view s_SourcesYaml_Source_Type = "Type"sv;
constexpr std::string_view s_SourcesYaml_Source_Arg = "Arg"sv;
constexpr std::string_view s_SourcesYaml_Source_Data = "Data"sv;
constexpr std::string_view s_SourcesYaml_Source_LastUpdate = "LastUpdate"sv;

constexpr std::string_view s_EmptySources = R"(
Sources:
)"sv;

constexpr std::string_view s_DefaultSourcesTombstoned = R"(
Sources:
  - Name: winget
    Type: ""
    Arg: ""
    Data: ""
    IsTombstone: true
  - Name: msstore
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

constexpr std::string_view s_SingleSourceMetadata = R"(
Sources:
  - Name: testName
    LastUpdate: 100
)"sv;

constexpr std::string_view s_SingleSourceMetadataUpdate = R"(
Sources:
  - Name: testName
    LastUpdate: 101
)"sv;

constexpr std::string_view s_DoubleSource = R"(
Sources:
  - Name: testName
    Type: testType
    Arg: testArg
    Data: testData
    IsTombstone: false
  - Name: testName2
    Type: testType
    Arg: testArg2
    Data: testData2
    IsTombstone: false
)"sv;

constexpr std::string_view s_DoubleSourceMetadata = R"(
Sources:
  - Name: testName
    LastUpdate: 100
  - Name: testName2
    LastUpdate: 200
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
  - Name: msstore
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
    Arg: https://cdn.winget.microsoft.com/cache
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

    // Failing source for use with s_TwoSource_AggregateSourceTest
    struct FailingSourcesTestSource : public TestSource
    {
        static constexpr HRESULT FailingHR = 0xBADDAD0D;

        FailingSourcesTestSource() = default;
        FailingSourcesTestSource(const SourceDetails& details)
        {
            Details = details;
        }

        static std::shared_ptr<ISource> CreateFailWinget(const SourceDetails& details)
        {
            if (details.Name == "winget")
            {
                THROW_HR(FailingHR);
            }

            return std::shared_ptr<ISource>(new FailingSourcesTestSource(details));
        }

        static std::shared_ptr<ISource> CreateFailAll(const SourceDetails&)
        {
            THROW_HR(FailingHR);
        }
    };

    void RequireDefaultSourcesAt(const std::vector<SourceDetails>& sources, size_t index)
    {
        REQUIRE(sources.size() >= index + c_DefaultSourceCount);

        for (size_t i = index; i < sources.size(); ++i)
        {
            INFO(i);
            REQUIRE(sources[i].Origin == SourceOrigin::Default);
        }
    }
}


TEST_CASE("RepoSources_UserSettingDoesNotExist", "[sources]")
{
    RemoveSetting(Stream::UserSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount);
    RequireDefaultSourcesAt(sources, 0);
}

TEST_CASE("RepoSources_EmptySourcesList", "[sources]")
{
    SetSetting(Stream::UserSources, s_EmptySources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount);
    RequireDefaultSourcesAt(sources, 0);
}

TEST_CASE("RepoSources_DefaultSourcesTombstoned", "[sources]")
{
    SetSetting(Stream::UserSources, s_DefaultSourcesTombstoned);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.empty());
}

TEST_CASE("RepoSources_SingleSource", "[sources]")
{
    SetSetting(Stream::UserSources, s_SingleSource);
    RemoveSetting(Stream::SourcesMetadata);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 1);

    REQUIRE(sources[0].Name == "testName");
    REQUIRE(sources[0].Type == "testType");
    REQUIRE(sources[0].Arg == "testArg");
    REQUIRE(sources[0].Data == "testData");
    REQUIRE(sources[0].Origin == SourceOrigin::User);
    REQUIRE(sources[0].LastUpdateTime == ConvertUnixEpochToSystemClock(0));

    RequireDefaultSourcesAt(sources, 1);
}

TEST_CASE("RepoSources_ThreeSources", "[sources]")
{
    SetSetting(Stream::UserSources, s_ThreeSources);
    SetSetting(Stream::SourcesMetadata, s_ThreeSourcesMetadata);

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
    SetSetting(Stream::UserSources, "Name: Value : BAD");

    REQUIRE_THROWS_HR(GetSources(), APPINSTALLER_CLI_ERROR_SOURCES_INVALID);
}

TEST_CASE("RepoSources_MissingField", "[sources]")
{
    SetSetting(Stream::UserSources, s_SingleSource_MissingArg);

    REQUIRE_THROWS_HR(GetSources(), APPINSTALLER_CLI_ERROR_SOURCES_INVALID);
}

TEST_CASE("RepoSources_AddSource", "[sources]")
{
    SetSetting(Stream::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    SourceDetails details;
    details.Name = "thisIsTheName";
    details.Type = "thisIsTheType";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";

    bool addCalledOnFactory = false;
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = details.Data; };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    ProgressCallback progress;
    AddSource(details, progress);

    REQUIRE(addCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 1);

    REQUIRE(sources[0].Name == details.Name);
    REQUIRE(sources[0].Type == details.Type);
    REQUIRE(sources[0].Arg == details.Arg);
    REQUIRE(sources[0].Data == details.Data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
    REQUIRE(sources[0].Origin == SourceOrigin::User);

    RequireDefaultSourcesAt(sources, 1);
}

TEST_CASE("RepoSources_AddMultipleSources", "[sources]")
{
    SetSetting(Stream::UserSources, s_EmptySources);

    SourceDetails details;
    details.Name = "thisIsTheName";
    details.Type = "thisIsTheType";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";

    const char* suffix[2] = { "", "2" };

    TestSourceFactory factory1{ SourcesTestSource::Create };
    factory1.OnAdd = [&](SourceDetails& sd) { sd.Data = details.Data; };
    TestHook_SetSourceFactoryOverride(details.Type, factory1);

    ProgressCallback progress;
    AddSource(details, progress);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 1);

    REQUIRE(sources[0].Name == details.Name);
    REQUIRE(sources[0].Type == details.Type);
    REQUIRE(sources[0].Arg == details.Arg);
    REQUIRE(sources[0].Data == details.Data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
    REQUIRE(sources[0].Origin == SourceOrigin::User);

    RequireDefaultSourcesAt(sources, 1);

    SourceDetails details2;
    details2.Name = details.Name + suffix[1];
    details2.Type = details.Type + suffix[1];
    details2.Arg = details.Arg + suffix[1];
    details2.Data = details.Data + suffix[1];
    TestSourceFactory factory2{ SourcesTestSource::Create };
    factory2.OnAdd = [&](SourceDetails& sd) { sd.Data = details2.Data; };
    TestHook_SetSourceFactoryOverride(details2.Type, factory2);

    AddSource(details2, progress);

    sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 2);

    for (size_t i = 0; i < 2; ++i)
    {
        INFO("Source #" << i);
        REQUIRE(sources[i].Name == details.Name + suffix[i]);
        REQUIRE(sources[i].Type == details.Type + suffix[i]);
        REQUIRE(sources[i].Arg == details.Arg + suffix[i]);
        REQUIRE(sources[i].Data == details.Data + suffix[i]);
        REQUIRE(sources[i].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
        REQUIRE(sources[i].Origin == SourceOrigin::User);
    }

    RequireDefaultSourcesAt(sources, 2);
}

TEST_CASE("RepoSources_UpdateSource", "[sources]")
{
    using namespace std::chrono_literals;

    SetSetting(Stream::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    SourceDetails details;
    details.Name = "thisIsTheName";
    details.Type = "thisIsTheType";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";

    bool addCalledOnFactory = false;
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = details.Data; };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    ProgressCallback progress;
    AddSource(details, progress);

    REQUIRE(addCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 1);

    REQUIRE(sources[0].Name == details.Name);
    REQUIRE(sources[0].Type == details.Type);
    REQUIRE(sources[0].Arg == details.Arg);
    REQUIRE(sources[0].Data == details.Data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
    REQUIRE(sources[0].Origin == SourceOrigin::User);

    RequireDefaultSourcesAt(sources, 1);

    // Reset for a call to update
    bool updateCalledOnFactory = false;
    auto now = std::chrono::system_clock::now();
    factory.OnUpdate = [&](const SourceDetails&) { updateCalledOnFactory = true; };

    UpdateSource(details.Name, progress);

    REQUIRE(updateCalledOnFactory);

    sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 1);

    REQUIRE(sources[0].Name == details.Name);
    REQUIRE(sources[0].Type == details.Type);
    REQUIRE(sources[0].Arg == details.Arg);
    REQUIRE(sources[0].Data == details.Data);
    REQUIRE((now - sources[0].LastUpdateTime) < 1s);
}

TEST_CASE("RepoSources_UpdateSourceRetries", "[sources]")
{
    using namespace std::chrono_literals;

    SetSetting(Stream::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    SourceDetails details;
    details.Name = "thisIsTheName";
    details.Type = "thisIsTheType";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";

    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnAdd = [&](SourceDetails& sd) { sd.Data = details.Data; };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    ProgressCallback progress;
    AddSource(details, progress);

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

    UpdateSource(details.Name, progress);

    REQUIRE(updateCalledOnFactoryAgain);
}

TEST_CASE("RepoSources_RemoveSource", "[sources]")
{
    SetSetting(Stream::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    SourceDetails details;
    details.Name = "thisIsTheName";
    details.Type = "thisIsTheType";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";

    bool removeCalledOnFactory = false;
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnRemove = [&](const SourceDetails&) { removeCalledOnFactory = true; };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    ProgressCallback progress;
    AddSource(details, progress);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 1);

    RemoveSource(details.Name, progress);

    REQUIRE(removeCalledOnFactory);

    sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount);
}

TEST_CASE("RepoSources_RemoveDefaultSource", "[sources]")
{
    SetSetting(Stream::UserSources, s_EmptySources);
    TestHook_ClearSourceFactoryOverrides();

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount);
    REQUIRE(sources[0].Origin == SourceOrigin::Default);

    bool removeCalledOnFactory = false;
    TestSourceFactory factory{ SourcesTestSource::Create };
    factory.OnRemove = [&](const SourceDetails&) { removeCalledOnFactory = true; };
    TestHook_SetSourceFactoryOverride(sources[0].Type, factory);

    ProgressCallback progress;

    RemoveSource(sources[0].Name, progress);

    REQUIRE(removeCalledOnFactory);

    sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount - 1);
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

    SetSetting(Stream::UserSources, s_SingleSource);

    ProgressCallback progress;
    auto source = OpenSource(name, progress);

    REQUIRE(updateCalledOnFactory);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount + 1);

    REQUIRE(sources[0].Name == name);
    REQUIRE(sources[0].Type == type);
    REQUIRE(sources[0].Arg == arg);
    REQUIRE(sources[0].Data == data);
    REQUIRE(sources[0].LastUpdateTime != ConvertUnixEpochToSystemClock(0));
}

TEST_CASE("RepoSources_DropSourceByName", "[sources]")
{
    SetSetting(Stream::UserSources, s_ThreeSources);
    SetSetting(Stream::SourcesMetadata, s_ThreeSourcesMetadata);

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
    SetSetting(Stream::UserSources, s_ThreeSources);

    std::vector<SourceDetails> sources = GetSources();
    REQUIRE(sources.size() == 3);

    DropSource({});

    sources = GetSources();
    REQUIRE(sources.size() == c_DefaultSourceCount);
    REQUIRE(sources[0].Origin == SourceOrigin::Default);
}

TEST_CASE("RepoSources_SearchAcrossMultipleSources", "[sources]")
{
    TestHook_ClearSourceFactoryOverrides();
    TestSourceFactory factory{ SourcesTestSource::Create };
    TestHook_SetSourceFactoryOverride("testType", factory);

    SetSetting(Stream::UserSources, s_TwoSource_AggregateSourceTest);

    ProgressCallback progress;
    auto source = OpenSource("", progress);

    SearchRequest request;
    auto result = source.Search(request);
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
    result = source.Search(request);
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
            SetSetting(Stream::UserSources, s_EmptySources);

            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount - 1);
        }
        SECTION("Add default source")
        {
            // We should not be able to add the default source manually.
            SetSetting(Stream::UserSources, s_EmptySources);

            ProgressCallback progress;
            SourceDetails details;
            details.Name = "winget";
            details.Type = "Microsoft.PreIndexed.Package";
            details.Arg = "https://cdn.winget.microsoft.com/cache";
            REQUIRE_POLICY_EXCEPTION(
                AddSource(details, progress),
                TogglePolicy::Policy::DefaultSource);
        }
        SECTION("Ignore default source from user")
        {
            // We should ignore any existing user source that is the same as the default.
            SetSetting(Stream::UserSources, s_DefaultSourceAsUserSource);

            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount - 1);
        }
        SECTION("Add same-name source from user")
        {
            // We should allow adding sources with the same name as the default but
            // pointing somewhere else.
            SetSetting(Stream::UserSources, s_EmptySources);
            TestHook_ClearSourceFactoryOverrides();

            SourceDetails details;
            details.Name = "winget";
            details.Type = "someType";
            details.Arg = "notWingetRealArg";
            details.Data = "someData";

            bool addCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnAdd = [&](SourceDetails& sd) { addCalledOnFactory = true; sd.Data = details.Data; };
            TestHook_SetSourceFactoryOverride(details.Type, factory);

            ProgressCallback progress;
            AddSource(details, progress);

            REQUIRE(addCalledOnFactory);

            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount);

            REQUIRE(sources[0].Name == details.Name);
            REQUIRE(sources[0].Type == details.Type);
            REQUIRE(sources[0].Arg == details.Arg);
            REQUIRE(sources[0].Data == details.Data);
            REQUIRE(sources[0].Origin == SourceOrigin::User);
        }
        SECTION("Allow same name source from user")
        {
            // We should respect existing user sources with the same name.
            // We should allow adding sources with the same name as the default but
            // pointing somewhere else.
            SetSetting(Stream::UserSources, s_UserSourceNamedLikeDefault);

            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount);

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
            SetSetting(Stream::UserSources, s_EmptySources);

            ProgressCallback progress;
            REQUIRE_POLICY_EXCEPTION(
                RemoveSource("winget", progress),
                TogglePolicy::Policy::DefaultSource);
        }
        SECTION("Tombstone is overridden")
        {
            // We should ignore if the default source was already deleted.
            SetSetting(Stream::UserSources, s_DefaultSourcesTombstoned);

            auto sources = GetSources();
            REQUIRE(sources.size() == 1);
            REQUIRE(sources[0].Name == "winget");
            REQUIRE(sources[0].Origin == SourceOrigin::Default);
        }
        SECTION("Same name source is overridden")
        {
            // We should ignore existing user sources with the same name as the default.
            SetSetting(Stream::UserSources, s_UserSourceNamedLikeDefault);

            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount);

            REQUIRE(sources[1].Name == "winget");
            REQUIRE(sources[1].Arg == "https://cdn.winget.microsoft.com/cache");
            REQUIRE(sources[1].Origin == SourceOrigin::Default);
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
            SetSetting(Stream::UserSources, s_EmptySources);

            auto sources = GetSources();

            // The source list includes the default source
            REQUIRE(sources.size() == policySources.size() + c_DefaultSourceCount);
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
            SetSetting(Stream::UserSources, s_SingleSource);

            auto sources = GetSources();

            // The source list includes the default source
            REQUIRE(sources.size() == c_DefaultSourceCount + 1);
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

            bool removeCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnRemove = [&](const SourceDetails&) { removeCalledOnFactory = true; };
            TestHook_SetSourceFactoryOverride(policySource.Type, factory);

            policies.SetValue<ValuePolicy::AdditionalSources>({ policySource });
            SetSetting(Stream::UserSources, s_EmptySources);

            ProgressCallback progress;
            REQUIRE_POLICY_EXCEPTION(
                RemoveSource(policySource.Name, progress),
                TogglePolicy::Policy::AdditionalSources);
            REQUIRE_FALSE(removeCalledOnFactory);
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
            SetSetting(Stream::UserSources, s_EmptySources);

            auto sources = GetSources();

            REQUIRE(sources.size() == c_DefaultSourceCount);
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
            SetSetting(Stream::UserSources, s_EmptySources);
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
            SourceDetails details;
            details.Name = policySource.Name;
            details.Type = policySource.Type;
            details.Arg = policySource.Arg;
            AddSource(details, progress);

            REQUIRE(addCalledOnFactory);

            // The source list includes the default source
            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount + 1);
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
            SetSetting(Stream::UserSources, s_EmptySources);

            ProgressCallback progress;
            SourceDetails details;
            details.Name = "notAllowed";
            details.Type = "type";
            details.Arg = "arg";

            bool addCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnAdd = [&](SourceDetails&) { addCalledOnFactory = true; };
            TestHook_SetSourceFactoryOverride(details.Type, factory);

            REQUIRE_POLICY_EXCEPTION(
                AddSource(details, progress),
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
            SetSetting(Stream::UserSources, s_EmptySources);

            ProgressCallback progress;
            SourceDetails details;
            details.Name = "name";
            details.Type = "type";
            details.Arg = "arg";

            bool addCalledOnFactory = false;
            TestSourceFactory factory{ SourcesTestSource::Create };
            factory.OnAdd = [&](SourceDetails&) { addCalledOnFactory = true; };
            TestHook_SetSourceFactoryOverride(details.Type, factory);

            REQUIRE_POLICY_EXCEPTION(
                AddSource(details, progress),
                TogglePolicy::Policy::AllowedSources);
            REQUIRE_FALSE(addCalledOnFactory);

            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount);
            REQUIRE(sources[0].Origin == SourceOrigin::Default);
        }
        SECTION("Existing sources are ignored")
        {
            SetSetting(Stream::UserSources, s_SingleSource);

            auto sources = GetSources();
            REQUIRE(sources.size() == c_DefaultSourceCount);
            REQUIRE(sources[0].Origin == SourceOrigin::Default);
        }
    }
}

TEST_CASE("RepoSources_OpenMultipleWithSingleFailure", "[sources]")
{
    TestHook_ClearSourceFactoryOverrides();
    TestSourceFactory factory{ FailingSourcesTestSource::CreateFailWinget };
    TestHook_SetSourceFactoryOverride("testType", factory);

    SetSetting(Stream::UserSources, s_TwoSource_AggregateSourceTest);

    ProgressCallback progress;
    auto result = OpenSource("", progress);

    REQUIRE(result);

    SearchResult searchResult = result.Search({});

    REQUIRE(searchResult.Failures.size() == 1);

    HRESULT openFailure = S_OK;
    try
    {
        std::rethrow_exception(searchResult.Failures[0].Exception);
    }
    catch (const wil::ResultException& re)
    {
        openFailure = re.GetErrorCode();
    }
    catch (...) {}

    REQUIRE(openFailure == FailingSourcesTestSource::FailingHR);
}

TEST_CASE("RepoSources_OpenMultipleWithTotalFailure", "[sources]")
{
    TestHook_ClearSourceFactoryOverrides();
    TestSourceFactory factory{ FailingSourcesTestSource::CreateFailAll };
    TestHook_SetSourceFactoryOverride("testType", factory);

    SetSetting(Stream::UserSources, s_TwoSource_AggregateSourceTest);

    ProgressCallback progress;
    REQUIRE_THROWS_HR(OpenSource("", progress), APPINSTALLER_CLI_ERROR_FAILED_TO_OPEN_ALL_SOURCES);
}

TEST_CASE("RepoSources_UpdateSettingsDuringAction_SourcesUpdate", "[sources]")
{
    SetSetting(Stream::UserSources, s_SingleSource);
    SetSetting(Stream::SourcesMetadata, s_SingleSourceMetadata);

    std::string userSourcesUpdate{ s_DoubleSource };
    std::string sourcesMetadataUpdate{ s_DoubleSourceMetadata };

    std::string singleSourceName = "testName";
    std::string doubleSourceName = "testName2";

    std::string unusedSourceName = "unusedName";
    std::string unusedSourceArg = "unusedArg";
    std::string testSourceType = "testType";

    TestHook_ClearSourceFactoryOverrides();
    TestSourceFactory factory{ FailingSourcesTestSource::CreateFailAll };
    auto settingsUpdate = [&](const AppInstaller::Repository::SourceDetails&) 
    {
        SetSetting(Stream::UserSources, userSourcesUpdate);
        SetSetting(Stream::SourcesMetadata, sourcesMetadataUpdate);
    };
    factory.OnAdd = settingsUpdate;
    factory.OnUpdate = settingsUpdate;
    factory.OnRemove = settingsUpdate;
    TestHook_SetSourceFactoryOverride(testSourceType, factory);

    ProgressCallback progress;

    SECTION("Add")
    {
        SourceDetails addedSource;
        addedSource.Name = unusedSourceName;
        addedSource.Type = testSourceType;
        addedSource.Arg = unusedSourceArg;
        AddSource(addedSource, progress);

        auto sources = GetSources();
        REQUIRE(sources.size() == 3 + c_DefaultSourceCount);

        REQUIRE(sources[0].Name == singleSourceName);
        REQUIRE(sources[1].Name == doubleSourceName);
        REQUIRE(sources[2].Name == addedSource.Name);
    }
    SECTION("Add conflicting")
    {
        SourceDetails addedSource;
        addedSource.Name = doubleSourceName;
        addedSource.Type = testSourceType;
        addedSource.Arg = unusedSourceArg;
        REQUIRE_THROWS_HR(AddSource(addedSource, progress), APPINSTALLER_CLI_ERROR_SOURCE_NAME_ALREADY_EXISTS);
    }
    SECTION("Update")
    {
        UpdateSource(singleSourceName, progress);

        auto sources = GetSources();
        REQUIRE(sources.size() == 2 + c_DefaultSourceCount);

        REQUIRE(sources[0].Name == singleSourceName);
        REQUIRE(sources[1].Name == doubleSourceName);
    }
    SECTION("Remove")
    {
        RemoveSource(singleSourceName, progress);

        auto sources = GetSources();
        REQUIRE(sources.size() == 1 + c_DefaultSourceCount);

        REQUIRE(sources[0].Name == doubleSourceName);
    }
    SECTION("Remove already removed")
    {
        userSourcesUpdate = s_EmptySources;
        sourcesMetadataUpdate = s_EmptySources;

        RemoveSource(singleSourceName, progress);

        auto sources = GetSources();
        REQUIRE(sources.size() == c_DefaultSourceCount);
    }
}

TEST_CASE("RepoSources_UpdateSettingsDuringAction_MetadataUpdate", "[sources]")
{
    SetSetting(Stream::UserSources, s_SingleSource);
    SetSetting(Stream::SourcesMetadata, s_SingleSourceMetadata);

    std::string sourcesMetadataUpdate{ s_SingleSourceMetadataUpdate };
    int64_t updateTime = 101;

    std::string singleSourceName = "testName";
    std::string doubleSourceName = "testName2";

    std::string unusedSourceName = "unusedName";
    std::string unusedSourceArg = "unusedArg";
    std::string testSourceType = "testType";

    TestHook_ClearSourceFactoryOverrides();
    TestSourceFactory factory{ FailingSourcesTestSource::CreateFailAll };
    auto settingsUpdate = [&](const AppInstaller::Repository::SourceDetails&)
    {
        SetSetting(Stream::SourcesMetadata, sourcesMetadataUpdate);
    };
    factory.OnAdd = settingsUpdate;
    factory.OnUpdate = settingsUpdate;
    factory.OnRemove = settingsUpdate;
    TestHook_SetSourceFactoryOverride(testSourceType, factory);

    ProgressCallback progress;

    SECTION("Add")
    {
        SourceDetails addedSource;
        addedSource.Name = unusedSourceName;
        addedSource.Type = testSourceType;
        addedSource.Arg = unusedSourceArg;
        AddSource(addedSource, progress);

        auto sources = GetSources();
        REQUIRE(sources.size() == 2 + c_DefaultSourceCount);

        REQUIRE(sources[0].Name == singleSourceName);
        REQUIRE(ConvertSystemClockToUnixEpoch(sources[0].LastUpdateTime) == updateTime);
        REQUIRE(sources[1].Name == addedSource.Name);
    }
    SECTION("Update")
    {
        UpdateSource(singleSourceName, progress);

        auto sources = GetSources();
        REQUIRE(sources.size() == 1 + c_DefaultSourceCount);

        REQUIRE(sources[0].Name == singleSourceName);
        REQUIRE(ConvertSystemClockToUnixEpoch(sources[0].LastUpdateTime) > updateTime);
    }
    SECTION("Remove")
    {
        RemoveSource(singleSourceName, progress);

        auto sources = GetSources();
        REQUIRE(sources.size() == c_DefaultSourceCount);
    }
}
