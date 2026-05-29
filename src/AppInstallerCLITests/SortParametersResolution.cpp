// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Integration tests for SortParameters(Context&) constructor.
// These exercise the full resolution chain: CLI args → user settings → query-aware default.
// Pure sort algorithm tests live in PackageTableSortHelper.cpp.
#include "pch.h"
#include "TestCommon.h"
#include "ExecutionContext.h"
#include "Workflows/PackageTableSortHelper.h"

using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Settings;
using namespace TestCommon;

TEST_CASE("ListSort_SortParameters_NoArgs_NoQuery_DefaultsByName", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };

    SortParameters params(context);

    REQUIRE(params.ShouldSort);
    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Name);
    REQUIRE(params.Direction == SortDirection::Ascending);
}

TEST_CASE("ListSort_SortParameters_WithQuery_PreservesRelevance", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Query, "firefox"sv);

    SortParameters params(context);

    REQUIRE(params.Fields.empty());
    REQUIRE_FALSE(params.ShouldSort);
}

TEST_CASE("ListSort_SortParameters_WithMultiQuery_PreservesRelevance", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::MultiQuery, "fire"sv);
    context.Args.AddArg(Args::Type::MultiQuery, "fox"sv);

    SortParameters params(context);

    REQUIRE(params.Fields.empty());
    REQUIRE_FALSE(params.ShouldSort);
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

TEST_CASE("ListSort_SortParameters_ExplicitRelevance_NoSort", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Sort, "relevance"sv);

    SortParameters params(context);

    REQUIRE(params.Fields.size() == 1);
    REQUIRE(params.Fields[0] == SortField::Relevance);
    REQUIRE_FALSE(params.ShouldSort);
}

TEST_CASE("ListSort_SortParameters_ExplicitSort_OverridesQuery", "[listsort]")
{
    TestUserSettings testSettings;

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Query, "firefox"sv);
    context.Args.AddArg(Args::Type::Sort, "name"sv);

    SortParameters params(context);

    // Explicit --sort overrides query-based relevance preservation
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
