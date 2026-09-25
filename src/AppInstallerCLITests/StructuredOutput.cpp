// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include <Commands/ListCommand.h>
#include <Commands/SearchCommand.h>
#include <Commands/UpgradeCommand.h>
#include <ExecutionReporter.h>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Settings;
using namespace std::string_view_literals;

namespace
{
    constexpr std::string_view s_CommonSchemaId = "https://aka.ms/winget-cli-output.common.1.0.schema.json";

    Json::Value LoadSchema(std::string_view fileName)
    {
        std::ifstream schemaStream{ TestCommon::TestDataFile{ fileName }.GetPath() };
        REQUIRE(schemaStream);

        std::stringstream schemaText;
        schemaText << schemaStream.rdbuf();
        return TestCommon::ConvertToJson(schemaText.str());
    }

    void ValidateAgainstSchema(const Json::Value& document, std::string_view schemaFileName)
    {
        using Adapter = valijson::adapters::JsonCppAdapter;

        Json::Value schemaJson = LoadSchema(schemaFileName);
        valijson::Schema schema;
        valijson::SchemaParser parser;
        parser.populateSchema(
            Adapter{ schemaJson },
            schema,
            [](const std::string& uri) -> const Json::Value*
            {
                if (uri == s_CommonSchemaId)
                {
                    return new Json::Value{ LoadSchema("common.1.0.schema.json") };
                }

                return nullptr;
            },
            [](const Json::Value* schemaDocument)
            {
                delete schemaDocument;
            });

        valijson::ValidationResults results;
        valijson::Validator validator;
        INFO("Schema validation failed for " << schemaFileName);
        REQUIRE(validator.validate(schema, Adapter{ document }, &results));
    }

    enum class StructuredOutputTestBehavior
    {
        Warning,
        Error,
        Termination,
    };

    struct StructuredOutputTestCommand : public Command
    {
        StructuredOutputTestCommand(StructuredOutputTestBehavior behavior) :
            Command("list", "", CommandOutputFlags::IgnoreSettingsWarnings), m_behavior(behavior)
        {
        }

        Resource::LocString ShortDescription() const override { return Resource::String::PendingWorkError; }
        Resource::LocString LongDescription() const override { return Resource::String::PendingWorkError; }

        std::optional<StructuredOutput::Mode> GetStructuredOutputMode(const Args&) const override
        {
            return StructuredOutput::Mode::Installed;
        }

    protected:
        void ExecuteInternal(Context& context) const override
        {
            switch (m_behavior)
            {
            case StructuredOutputTestBehavior::Warning:
                context.Reporter.AddStructuredOutputWarning("TestWarning", "Warning message.");
                break;
            case StructuredOutputTestBehavior::Error:
                context.Reporter.AddStructuredOutputError(APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED, "Error message.");
                break;
            case StructuredOutputTestBehavior::Termination:
                context.Terminate(APPINSTALLER_CLI_ERROR_COMMAND_FAILED);
                break;
            }
        }

    private:
        StructuredOutputTestBehavior m_behavior;
    };
}

TEST_CASE("StructuredOutput_PackageResult", "[StructuredOutput]")
{
    std::ostringstream output;
    Reporter reporter{ output, std::cin };
    reporter.BeginStructuredOutput("list", StructuredOutput::Mode::Installed);

    StructuredOutput::Package package;
    package.Name = "Example";
    package.Id = "Example.Package";
    package.InstalledVersion = "1.0";
    package.AvailableVersions = { "2.0", "1.0" };
    package.IsUpdateAvailable = true;
    package.Source = "winget";
    package.UpgradeVersion = "2.0";

    StructuredOutput::PackageResult result;
    result.Packages.emplace_back(std::move(package));
    reporter.SetStructuredOutputResult(std::move(result));
    reporter.FinalizeStructuredOutput();
    reporter.FinalizeStructuredOutput();

    Json::Value json = TestCommon::ConvertToJson(output.str());
    REQUIRE(json["$schema"].asString() == "https://aka.ms/winget-cli-output.list.1.0.schema.json");
    REQUIRE(json["schemaVersion"].asString() == "1.0");
    REQUIRE(json["command"].asString() == "list");
    REQUIRE(json["mode"].asString() == "installed");
    REQUIRE(json["result"]["packages"].size() == 1);
    REQUIRE(json["result"]["packages"][0]["AvailableVersions"].size() == 2);
    REQUIRE(json["result"]["packages"][0]["Source"].asString() == "winget");
    REQUIRE(json["result"]["packages"][0]["UpgradeVersion"].asString() == "2.0");
    REQUIRE(json["warnings"].empty());
    REQUIRE(json["errors"].empty());
    ValidateAgainstSchema(json, "list.1.0.schema.json");
}

TEST_CASE("StructuredOutput_PartialFailure", "[StructuredOutput]")
{
    std::ostringstream output;
    Reporter reporter{ output, std::cin };
    reporter.BeginStructuredOutput("upgrade", StructuredOutput::Mode::AvailableUpgrades);

    StructuredOutput::Package package;
    package.Name = "Example";
    package.Id = "Example.Package";
    package.InstalledVersion = "1.0";

    StructuredOutput::PackageResult result;
    result.Packages.emplace_back(std::move(package));
    result.Truncated = true;
    reporter.SetStructuredOutputResult(std::move(result));
    reporter.AddStructuredOutputWarning("SearchTruncated", "More results are available.");
    reporter.AddStructuredOutputError(APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED, "Failed to open source.", "winget");
    reporter.FinalizeStructuredOutput();

    Json::Value json = TestCommon::ConvertToJson(output.str());
    REQUIRE(json["mode"].asString() == "availableUpgrades");
    REQUIRE(json["result"]["truncated"].asBool());
    REQUIRE(json["warnings"][0]["code"].asString() == "SearchTruncated");
    REQUIRE(json["warnings"][0]["source"].isNull());
    REQUIRE(json["errors"][0]["code"].asString() == "0x8A150045");
    REQUIRE(json["errors"][0]["source"].asString() == "winget");
    REQUIRE(json["result"]["packages"][0]["Source"].isNull());
    REQUIRE(json["result"]["packages"][0]["UpgradeVersion"].isNull());
    REQUIRE(reporter.GetStructuredOutputError() == APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED);
    ValidateAgainstSchema(json, "upgrade.1.0.schema.json");
}

TEST_CASE("StructuredOutput_UnsupportedOperationEnvelope", "[StructuredOutput]")
{
    std::ostringstream output;
    Reporter reporter{ output, std::cin };
    reporter.BeginStructuredOutput("search", std::nullopt);
    reporter.AddStructuredOutputError(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, "Unsupported operation.");
    reporter.FinalizeStructuredOutput();

    Json::Value json = TestCommon::ConvertToJson(output.str());
    REQUIRE(json["$schema"].asString() == "https://aka.ms/winget-cli-output.error.1.0.schema.json");
    REQUIRE(json["command"].asString() == "search");
    REQUIRE(json["mode"].isNull());
    REQUIRE(json["result"].isNull());
    REQUIRE(json["errors"].size() == 1);
    ValidateAgainstSchema(json, "error.1.0.schema.json");
}

TEST_CASE("StructuredOutput_CommandSupport", "[StructuredOutput]")
{
    TestHook::SetSingleExperimentalFeature_Override featureOverride{ ExperimentalFeature::Feature::StructuredOutput };

    Execution::Args listArgs;
    listArgs.AddArg(Execution::Args::Type::OutputFormat, "json"sv);
    ListCommand list{ {} };
    REQUIRE_NOTHROW(list.ValidateArguments(listArgs));
    REQUIRE(list.GetStructuredOutputMode(listArgs) == StructuredOutput::Mode::Installed);

    Execution::Args listUpgradesArgs;
    listUpgradesArgs.AddArg(Execution::Args::Type::OutputFormat, "json"sv);
    listUpgradesArgs.AddArg(Execution::Args::Type::Upgrade);
    REQUIRE_NOTHROW(list.ValidateArguments(listUpgradesArgs));
    REQUIRE(list.GetStructuredOutputMode(listUpgradesArgs) == StructuredOutput::Mode::AvailableUpgrades);

    Execution::Args upgradeArgs;
    upgradeArgs.AddArg(Execution::Args::Type::OutputFormat, "json"sv);
    upgradeArgs.AddArg(Execution::Args::Type::All);
    UpgradeCommand upgrade{ {} };
    REQUIRE_THROWS_AS(upgrade.ValidateArguments(upgradeArgs), CommandException);

    Execution::Args searchArgs;
    searchArgs.AddArg(Execution::Args::Type::OutputFormat, "json"sv);
    SearchCommand search{ {} };
    REQUIRE_THROWS_AS(search.ValidateArguments(searchArgs), CommandException);

    Execution::Args invalidFormatArgs;
    invalidFormatArgs.AddArg(Execution::Args::Type::OutputFormat, "xml"sv);
    REQUIRE_THROWS_AS(list.ValidateArguments(invalidFormatArgs), CommandException);

    Execution::Args detailsArgs;
    detailsArgs.AddArg(Execution::Args::Type::OutputFormat, "json"sv);
    detailsArgs.AddArg(Execution::Args::Type::ListDetails);
    REQUIRE_FALSE(list.GetStructuredOutputMode(detailsArgs));
    REQUIRE_THROWS_AS(list.ValidateArguments(detailsArgs), CommandException);
}

TEST_CASE("StructuredOutput_ParsingErrorDoesNotDependOnFormatPosition", "[StructuredOutput]")
{
    ListCommand list{ {} };

    for (const std::vector<std::string>& values :
        {
            std::vector<std::string>{ "--format", "json", "--not-an-argument" },
            std::vector<std::string>{ "--not-an-argument", "--format", "json" },
        })
    {
        std::ostringstream output;
        Context context{ output, std::cin };
        auto invocationValues = values;
        Invocation invocation{ std::move(invocationValues) };

        REQUIRE_THROWS_AS(list.ParseArguments(invocation, context.Args), CommandException);
        list.ConfigureOutput(context);
        REQUIRE(context.Reporter.IsStructuredOutputEnabled());
    }
}

TEST_CASE("StructuredOutput_FeatureDisabledStillSelectsJson", "[StructuredOutput]")
{
    std::map<ExperimentalFeature::Feature, bool> overrides{ { ExperimentalFeature::Feature::StructuredOutput, false } };
    AppInstaller::Settings::SetExperimentalFeatureOverride(&overrides);
    auto resetOverride = wil::scope_exit([]()
    {
        AppInstaller::Settings::SetExperimentalFeatureOverride(nullptr);
    });

    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::OutputFormat, "json"sv);
    ListCommand list{ {} };
    list.ConfigureOutput(context);

    REQUIRE(context.Reporter.IsStructuredOutputEnabled());
    REQUIRE_THROWS_AS(list.ValidateArguments(context.Args), CommandException);
}

TEST_CASE("StructuredOutput_HelpKeepsTextOutput", "[StructuredOutput]")
{
    TestHook::SetSingleExperimentalFeature_Override featureOverride{ ExperimentalFeature::Feature::StructuredOutput };
    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::Help);
    context.Args.AddArg(Args::Type::OutputFormat, "json"sv);

    ListCommand list{ {} };
    list.ConfigureOutput(context);

    REQUIRE_FALSE(context.Reporter.IsStructuredOutputEnabled());
    REQUIRE_NOTHROW(list.ValidateArguments(context.Args));
}

TEST_CASE("StructuredOutput_ConfiguresNonInteractiveOutput", "[StructuredOutput]")
{
    TestHook::SetSingleExperimentalFeature_Override featureOverride{ ExperimentalFeature::Feature::StructuredOutput };
    std::ostringstream output;
    Context context{ output, std::cin };
    context.Args.AddArg(Args::Type::OutputFormat, "json"sv);
    context.Args.AddArg(Args::Type::NoVT);

    ListCommand list{ {} };
    list.ConfigureOutput(context);
    context.UpdateForArgs();

    REQUIRE(context.Reporter.IsStructuredOutputEnabled());
    REQUIRE(context.Args.Contains(Args::Type::DisableInteractivity));
}

TEST_CASE("StructuredOutput_ExecutionExitCodes", "[StructuredOutput]")
{
    SECTION("Warning keeps a successful exit code")
    {
        std::ostringstream output;
        Context context{ output, std::cin };
        context.Args.AddArg(Args::Type::OutputFormat, "json"sv);
        std::unique_ptr<Command> command = std::make_unique<StructuredOutputTestCommand>(StructuredOutputTestBehavior::Warning);
        context.SetExecutingCommand(command.get());
        command->ConfigureOutput(context);

        REQUIRE(Execute(context, command) == S_OK);

        Json::Value json = TestCommon::ConvertToJson(output.str());
        REQUIRE(json["warnings"][0]["code"].asString() == "TestWarning");
        REQUIRE(json["errors"].empty());
        ValidateAgainstSchema(json, "list.1.0.schema.json");
    }

    SECTION("The first structured error supplies the exit code")
    {
        std::ostringstream output;
        Context context{ output, std::cin };
        context.Args.AddArg(Args::Type::OutputFormat, "json"sv);
        std::unique_ptr<Command> command = std::make_unique<StructuredOutputTestCommand>(StructuredOutputTestBehavior::Error);
        context.SetExecutingCommand(command.get());
        command->ConfigureOutput(context);

        REQUIRE(Execute(context, command) == APPINSTALLER_CLI_ERROR_SOURCE_OPEN_FAILED);

        Json::Value json = TestCommon::ConvertToJson(output.str());
        REQUIRE(json["errors"][0]["code"].asString() == "0x8A150045");
        ValidateAgainstSchema(json, "list.1.0.schema.json");
    }

    SECTION("A terminating HRESULT is added to the document")
    {
        std::ostringstream output;
        Context context{ output, std::cin };
        context.Args.AddArg(Args::Type::OutputFormat, "json"sv);
        std::unique_ptr<Command> command = std::make_unique<StructuredOutputTestCommand>(StructuredOutputTestBehavior::Termination);
        context.SetExecutingCommand(command.get());
        command->ConfigureOutput(context);

        REQUIRE(Execute(context, command) == APPINSTALLER_CLI_ERROR_COMMAND_FAILED);

        Json::Value json = TestCommon::ConvertToJson(output.str());
        REQUIRE(json["errors"][0]["code"].asString() == "0x8A150003");
        ValidateAgainstSchema(json, "list.1.0.schema.json");
    }
}
