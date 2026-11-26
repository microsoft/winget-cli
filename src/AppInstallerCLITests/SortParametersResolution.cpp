// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Integration tests for SortParameters(Context&) constructor.
// Resolution: CLI args > user settings > optional Name override > relevance and name.
// Pure sort algorithm tests live in PackageTableSortHelper.cpp.
#include "pch.h"
#include "TestCommon.h"
#include "ExecutionContext.h"
#include "Workflows/PackageTableSortHelper.h"

using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Settings;
using namespace TestCommon;

TEST_CASE("ListSort_SortParameters_NoArgs_DefaultsByRelevanceAndName", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 2);
    REQUIRE(params.Fields[0] == SortField::Relevance);
    REQUIRE(params.Fields[1] == SortField::Name);
    REQUIRE(params.Direction == SortDirection::Ascending);
}

TEST_CASE("ListSort_SortParameters_WithQuery_DefaultsByRelevanceAndName", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Query, "firefox"sv);

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields == (std::vector<SortField>{ SortField::Relevance, SortField::Name }));
}

TEST_CASE("ListSort_SortParameters_WithMultiQuery_DefaultsByRelevanceAndName", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::MultiQuery, "fire"sv);
    context.Args.AddArg(Args::Type::MultiQuery, "fox"sv);

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields == (std::vector<SortField>{ SortField::Relevance, SortField::Name }));
}

TEST_CASE("ListSort_SortParameters_EmptySettings_UsesDefaultAndDirection", "[listsort]")
{
    TestUserSettings settings;
    settings.Set<Setting::OutputSortOrder>(std::vector<SortField>{});
    settings.Set<Setting::OutputSortDirection>(SortDirection::Descending);
    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Query, "Runtime"sv);

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields == (std::vector<SortField>{ SortField::Relevance, SortField::Name }));
    REQUIRE(params.Direction == SortDirection::Descending);
}

TEST_CASE("ListSort_SortParameters_NameWithoutQueryOverride", "[listsort]")
{
    enum class Input { None, Query, MultiQuery, NameFilter };
    auto input = GENERATE(Input::None, Input::Query, Input::MultiQuery, Input::NameFilter);
    TestUserSettings settings;
    settings.Set<Setting::OutputSortDirection>(SortDirection::Descending);
    std::ostringstream output;
    Context context{ output, std::cin };
    if (input != Input::None)
    {
        auto argument = input == Input::Query ? Args::Type::Query :
            input == Input::MultiQuery ? Args::Type::MultiQuery : Args::Type::Name;
        context.Args.AddArg(argument, "Runtime"sv);
    }

    std::vector<SortField> expected = input == Input::None || input == Input::NameFilter ?
        std::vector{ SortField::Name } : std::vector{ SortField::Relevance, SortField::Name };
    auto expectedDirection = SortDirection::Descending;
    SECTION("Built-in order")
    {
    }
    SECTION("Empty settings use the built-in order")
    {
        settings.Set<Setting::OutputSortOrder>(std::vector<SortField>{});
    }
    SECTION("Settings override the built-in order")
    {
        settings.Set<Setting::OutputSortOrder>(std::vector{ SortField::Version });
        expected = { SortField::Version };
    }
    SECTION("Parameters override settings and the built-in order")
    {
        settings.Set<Setting::OutputSortOrder>(std::vector{ SortField::Version });
        context.Args.AddArg(Args::Type::Sort, "id"sv);
        context.Args.AddArg(Args::Type::SortAscending);
        expected = { SortField::Id };
        expectedDirection = SortDirection::Ascending;
    }

    SortParameters params(context, SortParameters::DefaultOrder::NameWithoutQuery);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields == expected);
    REQUIRE(params.Direction == expectedDirection);
}

TEST_CASE("ListSort_SortParameters_ExplicitSort_OverridesDefault", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Sort, "id"sv);

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Id);
    REQUIRE(params.Direction == SortDirection::Ascending);
}

TEST_CASE("ListSort_SortParameters_ExplicitSort_MultiField", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Sort, "source"sv);
    context.Args.AddArg(Args::Type::Sort, "name"sv);

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 2);
    REQUIRE(params.Fields[0] == SortField::Source);
    REQUIRE(params.Fields[1] == SortField::Name);
    REQUIRE(params.Direction == SortDirection::Ascending);
}

TEST_CASE("ListSort_SortParameters_ExplicitSort_Descending", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Sort, "name"sv);
    context.Args.AddArg(Args::Type::SortDescending);

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Name);
    REQUIRE(params.Direction == SortDirection::Descending);
}

TEST_CASE("ListSort_SortParameters_ExplicitRelevance", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Sort, "relevance"sv);

    SortParameters params(context);

    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Relevance);
    REQUIRE(params.ShouldSort);
}

TEST_CASE("ListSort_SortParameters_ExplicitSort_OverridesQuery", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Query, "firefox"sv);
    context.Args.AddArg(Args::Type::Sort, "name"sv);

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Name);
    REQUIRE(params.Direction == SortDirection::Ascending);
}

TEST_CASE("ListSort_SortParameters_Settings_SortOrder", "[listsort]")
{
    TestUserSettings testSettings;
    testSettings.Set<Setting::OutputSortOrder>(std::vector<SortField>{ SortField::Version });

    std::ostringstream output;
    Context context{ output, std::cin };

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Version);
    REQUIRE(params.Direction == SortDirection::Ascending);
}

TEST_CASE("ListSort_SortParameters_Settings_DescendingDirection", "[listsort]")
{
    TestUserSettings testSettings;
    testSettings.Set<Setting::OutputSortOrder>(std::vector<SortField>{ SortField::Name });
    testSettings.Set<Setting::OutputSortDirection>(SortDirection::Descending);

    std::ostringstream output;
    Context context{ output, std::cin };

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Name);
    REQUIRE(params.Direction == SortDirection::Descending);
}

TEST_CASE("ListSort_SortParameters_CLIArgs_OverrideSettings", "[listsort]")
{
    // Settings say sort by version
    TestUserSettings testSettings;
    testSettings.Set<Setting::OutputSortOrder>(std::vector<SortField>{ SortField::Version });

    std::ostringstream output;
    Context context{ output, std::cin };
    // CLI says sort by id
    context.Args.AddArg(Args::Type::Sort, "id"sv);

    SortParameters params(context);

    // CLI args win
    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Id);
    REQUIRE(params.Direction == SortDirection::Ascending);
}

TEST_CASE("ListSort_SortParameters_CLIDirection_OverridesSettings", "[listsort]")
{
    // Settings say descending
    TestUserSettings testSettings;
    testSettings.Set<Setting::OutputSortDirection>(SortDirection::Descending);

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Sort, "name"sv);
    context.Args.AddArg(Args::Type::SortAscending);

    SortParameters params(context);

    // CLI --ascending overrides settings descending
    REQUIRE(params.Direction == SortDirection::Ascending);
}
