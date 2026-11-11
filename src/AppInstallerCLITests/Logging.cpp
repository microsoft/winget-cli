// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSettings.h"
#include "WorkflowCommon.h"
#include <AppInstallerDownloader.h>
#include <AppInstallerFileLogger.h>
#include <AppInstallerStrings.h>
#include <AppInstallerTelemetry.h>
#include <Commands/DscAdminSettingsResource.h>
#include <Commands/RootCommand.h>
#include <winget/ThreadGlobals.h>
#include <winget/NetworkSettings.h>
#include <winget/AdminSettings.h>
#include <winget/HttpClientHelper.h>
#include <tuple>
#include <wininet.h>

using namespace AppInstaller::Logging;
using namespace AppInstaller::Utility;
using namespace TestCommon;

namespace
{
    constexpr AppInstaller::Logging::Channel DefaultChannel = AppInstaller::Logging::Channel::Core;
    constexpr AppInstaller::Logging::Level DefaultLevel = AppInstaller::Logging::Level::Info;
}

TEST_CASE("NetworkSettings_ProxyLogRedactsCredentials", "[logging]")
{
    const auto testCase = GENERATE(
        std::pair{ "https://ProxyUser:ProxyPassword@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/" },
        std::pair{ "https://proxy.invalid:8080", "https://proxy.invalid:8080/" },
        std::pair{ "HTTPS://Proxy.Invalid:443", "https://proxy.invalid/" },
        std::pair{ "https://ProxyUser:ProxyPassword@[2001:db8::1]:8080/path@tag?x=@tag#fragment",
            "https://[redacted]@[2001:db8::1]:8080/path@tag?x=@tag#fragment" },
        std::pair{ "https://:ProxyPassword%40tail@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/" },
        std::pair{ "ProxyUser:ProxyPassword@proxy.invalid:8080", "[invalid proxy URI]" },
        std::pair{ "https://ProxyUser:ProxyPassword@proxy.invalid:bad", "[invalid proxy URI]" },
        std::pair{ "", "[invalid proxy URI]" },
        std::pair{ "https://ProxyUser:ProxyPassword@tail@proxy.invalid:8080", "[invalid proxy URI]" });
    const std::string proxyUri = testCase.first;
    const std::string expectedProxyUri = testCase.second;
    CAPTURE(proxyUri);

    TestUserSettings userSettings;
    TempFile logFile{ "NetworkSettings_ProxyLog", ".log" };
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();

    struct TestNetworkSettings : AppInstaller::Settings::NetworkSettings {};
    TestNetworkSettings networkSettings;

    Log().SetEnabledChannels(DefaultChannel);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    networkSettings.SetProxyUri(proxyUri);
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(networkSettings.GetProxyUri() == proxyUri);
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    REQUIRE(logContents.find("New value for proxy is " + expectedProxyUri + "\r\n") != std::string::npos);
    REQUIRE(logContents.find("ProxyUser") == std::string::npos);
    REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("NetworkSettings_DefaultProxyLogRedactsCredentials", "[logging]")
{
    const auto testCase = GENERATE(
        std::pair{ std::optional<std::string>{ "https://ProxyUser:ProxyPassword@proxy.invalid:8080" },
            "https://[redacted]@proxy.invalid:8080/" },
        std::pair{ std::optional<std::string>{ "https://proxy.invalid:8080" }, "https://proxy.invalid:8080/" },
        std::pair{ std::optional<std::string>{}, "not set" });
    const auto& proxyUri = testCase.first;
    const std::string expectedProxyUri = testCase.second;
    CAPTURE(proxyUri);

    GroupPolicyTestOverride policies;
    RemoveSetting(AppInstaller::Settings::Stream::AdminSettings);
    auto removeAdminSettings = wil::scope_exit([]() { RemoveSetting(AppInstaller::Settings::Stream::AdminSettings); });
    if (proxyUri)
    {
        REQUIRE(AppInstaller::Settings::SetAdminSetting(AppInstaller::Settings::StringAdminSetting::DefaultProxy, proxyUri.value()));
    }

    TestUserSettings userSettings;
    TempFile logFile{ "NetworkSettings_DefaultProxyLog", ".log" };
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();

    Log().SetEnabledChannels(DefaultChannel);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    struct TestNetworkSettings : AppInstaller::Settings::NetworkSettings {};
    TestNetworkSettings networkSettings;
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(networkSettings.GetProxyUri() == proxyUri);
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find("Default proxy is " + expectedProxyUri + "\r\n") != std::string::npos);
    REQUIRE(logContents.find("ProxyUser") == std::string::npos);
    REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("WinINet_ProxyLogRedactsCredentials", "[logging]")
{
    const auto testCase = GENERATE(
        std::tuple{ "https://ProxyUser:ProxyPassword@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/", E_INVALIDARG },
        std::tuple{ "https://proxy.invalid:8080", "https://proxy.invalid:8080/", HRESULT_FROM_WIN32(ERROR_INTERNET_UNRECOGNIZED_SCHEME) });
    const std::string proxyUri = std::get<0>(testCase);
    const std::string expectedProxyUri = std::get<1>(testCase);
    CAPTURE(proxyUri);

    TestUserSettings userSettings;
    TempFile logFile{ "WinINet_ProxyLog", ".log" };
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();

    auto& networkSettings = AppInstaller::Settings::Network();
    const auto previousProxyUri = networkSettings.GetProxyUri();
    auto restoreProxyUri = wil::scope_exit([&]() { networkSettings.SetProxyUri(previousProxyUri); });
    networkSettings.SetProxyUri(proxyUri);

    Log().SetEnabledChannels(DefaultChannel);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    std::ostringstream destination;
    AppInstaller::ProgressCallback progress;
    REQUIRE_THROWS_HR(DownloadToStream("invalid://proxy-log-test", destination, DownloadType::Manifest, progress),
        std::get<2>(testCase));
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(networkSettings.GetProxyUri() == proxyUri);
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find("Using proxy " + expectedProxyUri + "\r\n") != std::string::npos);
    REQUIRE(logContents.find("ProxyUser") == std::string::npos);
    REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("HttpClientHelper_ProxyLogRedactsCredentials", "[logging]")
{
    const auto testCase = GENERATE(
        std::pair{ std::optional<std::string>{ "https://ProxyUser:ProxyPassword@proxy.invalid:8080" },
            "Setting proxy for REST HTTP Client helper to https://[redacted]@proxy.invalid:8080/" },
        std::pair{ std::optional<std::string>{ "https://proxy.invalid:8080" },
            "Setting proxy for REST HTTP Client helper to https://proxy.invalid:8080/" },
        std::pair{ std::optional<std::string>{}, "REST HTTP Client helper does not use proxy" });
    const auto& proxyUri = testCase.first;
    const std::string expectedLog = testCase.second;
    CAPTURE(proxyUri);

    TestUserSettings userSettings;
    TempFile logFile{ "HttpClientHelper_ProxyLog", ".log" };
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();

    auto& networkSettings = AppInstaller::Settings::Network();
    const auto previousProxyUri = networkSettings.GetProxyUri();
    auto restoreProxyUri = wil::scope_exit([&]() { networkSettings.SetProxyUri(previousProxyUri); });
    networkSettings.SetProxyUri(proxyUri);

    Log().SetEnabledChannels(AppInstaller::Logging::Channel::Repo);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    REQUIRE_NOTHROW(AppInstaller::Http::HttpClientHelper{});
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(networkSettings.GetProxyUri() == proxyUri);
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find(expectedLog + "\r\n") != std::string::npos);
    REQUIRE(logContents.find("ProxyUser") == std::string::npos);
    REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("DscAdminSettings_ProxyLogRedactsCredentials", "[logging]")
{
    const auto testCase = GENERATE(
        std::pair{ std::optional<std::string>{ "https://ProxyUser:ProxyPassword@proxy.invalid:8080" },
            "`https://[redacted]@proxy.invalid:8080/`" },
        std::pair{ std::optional<std::string>{ "https://proxy.invalid:8080" }, "`https://proxy.invalid:8080/`" },
        std::pair{ std::optional<std::string>{ "https://ProxyUser:ProxyPassword@proxy.invalid:bad" }, "`[invalid proxy URI]`" },
        std::pair{ std::optional<std::string>{}, "null" });
    const auto& proxyUri = testCase.first;
    const std::string propertyName = GENERATE("DefaultProxy", "dEfAuLtPrOxY");
    const std::string expectedLog = "String admin setting: " + propertyName + " => " + testCase.second;
    CAPTURE(proxyUri, propertyName);

    GroupPolicyTestOverride policies;
    RemoveSetting(AppInstaller::Settings::Stream::AdminSettings);
    auto removeAdminSettings = wil::scope_exit([]() { RemoveSetting(AppInstaller::Settings::Stream::AdminSettings); });
    if (proxyUri)
    {
        REQUIRE(AppInstaller::Settings::SetAdminSetting(AppInstaller::Settings::StringAdminSetting::DefaultProxy, proxyUri.value()));
    }

    TestUserSettings userSettings;
    Json::Value input{ Json::objectValue };
    input["settings"][propertyName] = proxyUri ? Json::Value{ proxyUri.value() } : Json::Value{};
    std::istringstream inputStream{ Json::writeString(Json::StreamWriterBuilder{}, input) };
    std::ostringstream output;
    TestContext context{ output, inputStream };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(AppInstaller::CLI::Execution::Args::Type::DscResourceFunctionTest);

    TempFile logFile{ "DscAdminSettings_ProxyLog", ".log" };
    Log().SetEnabledChannels(AppInstaller::Logging::Channel::Config);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    AppInstaller::CLI::DscAdminSettingsResource command({});
    command.Execute(context);
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(context.GetTerminationHR() == S_OK);
    std::istringstream outputStream{ output.str() };
    std::string outputLine;
    REQUIRE(static_cast<bool>(std::getline(outputStream, outputLine)));
    const auto result = ConvertToJson(outputLine);
    REQUIRE(result["_inDesiredState"].asBool());
    REQUIRE(result["settings"]["DefaultProxy"] == input["settings"][propertyName]);

    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find(expectedLog + "\r\n") != std::string::npos);
    REQUIRE(logContents.find("ProxyUser") == std::string::npos);
    REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("DscInputJson_ProxyLogRedactsCredentials", "[logging]")
{
    struct InputCommand : AppInstaller::CLI::DscAdminSettingsResource
    {
        using DscAdminSettingsResource::DscAdminSettingsResource;
        using DscCommandBase::GetJsonFromInput;
    };

    Json::Value input;
    Json::Value expectedInput;
    SECTION("Proxy fields")
    {
        const auto testCase = GENERATE(
            std::pair{ R"("https://ProxyUser:ProxyPassword@proxy.invalid:8080")", R"("https://[redacted]@proxy.invalid:8080/")" },
            std::pair{ R"("https://proxy.invalid:8080")", R"("https://proxy.invalid:8080/")" },
            std::pair{ R"("https://ProxyUser:ProxyPassword@proxy.invalid:bad")", R"("[invalid proxy URI]")" },
            std::pair{ "null", "null" },
            std::pair{ R"(["ProxyUser","ProxyPassword"])", R"("[invalid proxy URI]")" },
            std::pair{ R"({"secret":"ProxyPassword"})", R"("[invalid proxy URI]")" },
            std::pair{ "42", R"("[invalid proxy URI]")" },
            std::pair{ "true", R"("[invalid proxy URI]")" });
        const std::string propertyName = GENERATE("DefaultProxy", "dEfAuLtPrOxY");
        CAPTURE(testCase.first, propertyName);
        input["settings"][propertyName] = ConvertToJson(testCase.first);
        input["settings"]["LocalManifestFiles"] = true;
        input["unrelated"] = "https://UnrelatedUser:UnrelatedPassword@other.invalid";
        expectedInput = input;
        expectedInput["settings"][propertyName] = ConvertToJson(testCase.second);
    }
    SECTION("Other input shapes")
    {
        const auto json = GENERATE("{}", "[]", R"({"settings":[]})",
            R"({"settings":{"OtherSetting":"https://UnrelatedUser:UnrelatedPassword@other.invalid"}})");
        CAPTURE(json);
        input = ConvertToJson(json);
        expectedInput = input;
    }

    TestUserSettings userSettings;
    std::istringstream inputStream{ Json::writeString(Json::StreamWriterBuilder{}, input) };
    std::ostringstream output;
    TestContext context{ output, inputStream };
    auto previousThreadGlobals = context.SetForCurrentThread();
    TempFile logFile{ "DscInputJson_ProxyLog", ".log" };
    Log().SetEnabledChannels(AppInstaller::Logging::Channel::CLI);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    InputCommand command("");
    const auto result = command.GetJsonFromInput(context);
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(result.has_value());
    REQUIRE(result.value() == input);
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    auto expectedJson = Json::writeString(Json::StreamWriterBuilder{}, expectedInput);
    AppInstaller::Utility::FindAndReplace(expectedJson, "\n", "\r\n");
    REQUIRE(logContents.find(expectedJson) != std::string::npos);
    REQUIRE(logContents.find("ProxyUser") == std::string::npos);
    REQUIRE(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("Invocation_ProxyLogRedactsCredentials", "[logging]")
{
    using ProxyValues = std::vector<std::pair<std::string, std::string>>;
    auto proxies = GENERATE(
        ProxyValues{ { "https://ProxyUser:ProxyPassword@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/" } },
        ProxyValues{ { "https://proxy.invalid:8080", "https://proxy.invalid:8080/" } },
        ProxyValues{},
        ProxyValues{
            { "https://ProxyUser:ProxyPassword@first.invalid:8080", "https://[redacted]@first.invalid:8080/" },
            { "https://OtherProxyUser:OtherProxyPassword@second.invalid:8080", "https://[redacted]@second.invalid:8080/" } },
        ProxyValues{
            { "https://ProxyUser:ProxyPassword@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/" },
            { "https://ProxyUser:ProxyPassword@proxy.invalid:8080/path", "https://[redacted]@proxy.invalid:8080/path" } },
        ProxyValues{
            { "https://proxy.invalid:8080", "https://proxy.invalid:8080/" },
            { "https://other.invalid:8080", "https://other.invalid:8080/" },
            { "https://proxy.invalid:8080", "https://proxy.invalid:8080/" } },
        ProxyValues{
            { "", "" },
            { "https://OtherProxyUser:OtherProxyPassword@proxy.invalid:8080", "https://[redacted]@proxy.invalid:8080/" } });
    const bool reverseOrder = GENERATE(false, true);
    if (reverseOrder)
    {
        std::reverse(proxies.begin(), proxies.end());
    }
    const bool inlineValue = GENERATE(false, true);
    CAPTURE(proxies, reverseOrder, inlineValue);
    std::vector<std::string> values{ "install", "--help" };
    std::vector<std::string> expectedProxies;
    std::string expectedLog = "WinGet invoked with arguments: 'install' '--help'";
    std::string commandLine = "\"winget client.exe\"  install\t--help";
    std::string expectedCommandLine = commandLine;
    for (const auto& [proxyUri, proxyForLog] : proxies)
    {
        expectedProxies.emplace_back(proxyUri);
        if (inlineValue)
        {
            values.emplace_back("--proxy=\"" + proxyUri + "\"");
            expectedLog += " '--proxy=\"" + proxyForLog + "\"'";
            commandLine += "  --proxy=\"" + proxyUri + "\"";
            expectedCommandLine += "  --proxy=\"" + proxyForLog + "\"";
        }
        else
        {
            values.emplace_back("--proxy");
            values.emplace_back(proxyUri);
            expectedLog += " '--proxy' '" + proxyForLog + "'";
            commandLine += "  --proxy  \"" + proxyUri + "\"";
            expectedCommandLine += "  --proxy  \"" + proxyForLog + "\"";
        }
    }

    TestUserSettings userSettings;
    GroupPolicyTestOverride policies;
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();
    const auto originalValues = values;
    AppInstaller::CLI::Invocation invocation{ std::move(values) };
    AppInstaller::CLI::RootCommand root;
    const auto command = root.FindSubCommand(invocation);
    REQUIRE(command);
    AppInstaller::CLI::Execution::Args args;
    REQUIRE_NOTHROW(command->ParseArguments(invocation, args));

    TempFile logFile{ "Invocation_ProxyLog", ".log" };
    Log().SetEnabledChannels(AppInstaller::Logging::Channel::CLI | AppInstaller::Logging::Channel::Core);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    invocation.Log(args, ConvertToUTF16(commandLine));
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    CHECK(invocation.GetArgs() == originalValues);
    CHECK(args.GetCount(AppInstaller::CLI::Execution::Args::Type::Proxy) == expectedProxies.size());
    if (!expectedProxies.empty())
    {
        const auto* parsedProxies = args.GetArgs(AppInstaller::CLI::Execution::Args::Type::Proxy);
        REQUIRE(parsedProxies);
        CHECK(*parsedProxies == expectedProxies);
    }
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    CHECK(logContents.find(expectedLog + "\r\n") != std::string::npos);
    CHECK(logContents.find("Command line Args: " + expectedCommandLine + "\r\n") != std::string::npos);
    CHECK(logContents.find("ProxyUser") == std::string::npos);
    CHECK(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("Invocation_DefaultProxyLogRedactsCredentials", "[logging]")
{
    using namespace AppInstaller::CLI;
    const auto setting = GENERATE(
        std::pair{ "DefaultProxy", true },
        std::pair{ "dEfAuLtPrOxY", true },
        std::pair{ "OtherSetting", false });
    const bool valueBeforeName = GENERATE(false, true);
    CAPTURE(setting, valueBeforeName);
    const std::string proxy = "https://ProxyUser:ProxyPassword@proxy.invalid:8080";
    std::vector<std::string> values = valueBeforeName ?
        std::vector<std::string>{ "settings", "set", "--value=" + proxy, "--setting", setting.first } :
        std::vector<std::string>{ "settings", "set", setting.first, proxy };
    std::string commandLine = "winget";
    std::string expectedLog = "WinGet invoked with arguments:";
    for (const auto& value : values)
    {
        commandLine += " " + value;
        expectedLog += " '" + value + "'";
    }
    std::string expectedCommandLine = commandLine;
    if (setting.second)
    {
        FindAndReplace(expectedCommandLine, proxy, "https://[redacted]@proxy.invalid:8080/");
        FindAndReplace(expectedLog, proxy, "https://[redacted]@proxy.invalid:8080/");
    }

    TestUserSettings userSettings;
    GroupPolicyTestOverride policies;
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();
    const auto originalValues = values;
    Invocation invocation{ std::move(values) };
    RootCommand root;
    const auto settingsCommand = root.FindSubCommand(invocation);
    REQUIRE(settingsCommand);
    const auto command = settingsCommand->FindSubCommand(invocation);
    REQUIRE(command);
    Execution::Args args;
    REQUIRE_NOTHROW(command->ParseArguments(invocation, args));

    TempFile logFile{ "Invocation_DefaultProxyLog", ".log" };
    Log().SetEnabledChannels(AppInstaller::Logging::Channel::CLI | AppInstaller::Logging::Channel::Core);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    invocation.Log(args, ConvertToUTF16(commandLine));
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    CHECK(invocation.GetArgs() == originalValues);
    CHECK(args.GetArg(Execution::Args::Type::SettingName) == setting.first);
    CHECK(args.GetArg(Execution::Args::Type::SettingValue) == proxy);
    CHECK_FALSE(args.Contains(Execution::Args::Type::Proxy));
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    CHECK(logContents.find(expectedLog + "\r\n") != std::string::npos);
    CHECK(logContents.find("Command line Args: " + expectedCommandLine + "\r\n") != std::string::npos);
    if (setting.second)
    {
        CHECK(logContents.find("ProxyUser") == std::string::npos);
        CHECK(logContents.find("ProxyPassword") == std::string::npos);
    }
}

TEST_CASE("Invocation_CompletionProxyLogRedactsCredentials", "[logging][complete]")
{
    using namespace AppInstaller::CLI;
    const bool inlineValue = GENERATE(false, true);
    const bool includeOuterProxy = GENERATE(false, true);
    const std::string proxyUri = "https://ProxyUser:ProxyPassword@proxy.invalid:8080";
    const std::string proxyForLog = "https://[redacted]@proxy.invalid:8080/";
    const std::string proxyArgument = (inlineValue ? "--proxy=" : "--proxy ") + proxyUri;
    const std::string proxyArgumentForLog = (inlineValue ? "--proxy=" : "--proxy ") + proxyForLog;
    using CompletionInput = std::tuple<std::string, std::string, std::string, std::string>;
    const auto input = GENERATE_COPY(
        CompletionInput{ "--he", "winget install " + proxyArgument + " --he",
            "--he", "winget install " + proxyArgumentForLog + " --he" },
        CompletionInput{ inlineValue ? proxyArgument : proxyUri, "winget install " + proxyArgument,
            inlineValue ? proxyArgumentForLog : proxyForLog, "winget install " + proxyArgumentForLog },
        CompletionInput{ "--he", "winget install --he " + proxyArgument,
            "--he", "winget install --he " + proxyArgumentForLog },
        CompletionInput{ "", "winget install " + proxyArgument,
            "", "winget install " + proxyArgumentForLog },
        CompletionInput{ "--he", "winget source add " + proxyArgument + " --he",
            "--he", "winget source add " + proxyArgumentForLog + " --he" },
        CompletionInput{ "--he", "winget install " + proxyArgument + " " + proxyArgument + "/path " + proxyArgument + " --he",
            "--he", "winget install " + proxyArgumentForLog + " " + proxyArgumentForLog + "path " + proxyArgumentForLog + " --he" },
        CompletionInput{ "--he", "winget install --source " + proxyArgument + " --he",
            "--he", "winget install --source " + proxyArgument + " --he" },
        CompletionInput{ proxyUri, "winget settings set DefaultProxy " + proxyUri,
            proxyForLog, "winget settings set DefaultProxy " + proxyForLog },
        CompletionInput{ "--he", "winget settings set dEfAuLtPrOxY " + proxyUri + " --he",
            "--he", "winget settings set dEfAuLtPrOxY " + proxyForLog + " --he" },
        CompletionInput{ "--value=" + proxyUri, "winget settings set --value=" + proxyUri + " --setting DefaultProxy",
            "--value=" + proxyForLog, "winget settings set --value=" + proxyForLog + " --setting DefaultProxy" },
        CompletionInput{ "--he", "winget settings set --value=" + proxyUri + " --he --setting DefaultProxy",
            "--he", "winget settings set --value=" + proxyForLog + " --he --setting DefaultProxy" },
        CompletionInput{ proxyUri, "winget settings set OtherSetting " + proxyUri,
            proxyUri, "winget settings set OtherSetting " + proxyUri },
        CompletionInput{ "--he", "winget install --he", "--he", "winget install --he" });
    const auto& [word, nestedCommandLine, expectedWord, expectedNestedCommandLine] = input;
    const auto cursor = word.empty() ?
        nestedCommandLine.find(inlineValue ? "--proxy" : proxyUri) :
        nestedCommandLine.rfind(word) + word.size();
    const auto position = std::to_string(cursor);
    CAPTURE(inlineValue, includeOuterProxy, word, nestedCommandLine, position);
    std::vector<std::string> values{ "complete", "--word", word, "--commandline", nestedCommandLine, "--position", position };
    std::string expectedLog = "WinGet invoked with arguments: 'complete' '--word' '" + expectedWord +
        "' '--commandline' '" + expectedNestedCommandLine + "' '--position' '" + position + "'";
    std::string commandLine = "winget complete --word \"" + word + "\" --commandline \"" + nestedCommandLine + "\" --position " + position;
    std::string expectedCommandLine = "winget complete --word \"" + expectedWord +
        "\" --commandline \"" + expectedNestedCommandLine + "\" --position " + position;
    const std::string outerProxy = "https://OuterProxyUser:OuterProxyPassword@outer.invalid:8181";
    if (includeOuterProxy)
    {
        values.emplace_back("--proxy");
        values.emplace_back(outerProxy);
        expectedLog += " '--proxy' 'https://[redacted]@outer.invalid:8181/'";
        commandLine += " --proxy " + outerProxy;
        expectedCommandLine += " --proxy https://[redacted]@outer.invalid:8181/";
    }

    TestUserSettings userSettings;
    GroupPolicyTestOverride policies;
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();
    const auto originalValues = values;
    Invocation invocation{ std::move(values) };
    RootCommand root;
    const auto command = root.FindSubCommand(invocation);
    REQUIRE(command);
    Execution::Args args;
    REQUIRE_NOTHROW(command->ParseArguments(invocation, args));

    TempFile logFile{ "Invocation_CompletionProxyLog", ".log" };
    Log().SetEnabledChannels(AppInstaller::Logging::Channel::CLI | AppInstaller::Logging::Channel::Core);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    REQUIRE_NOTHROW(invocation.Log(args, ConvertToUTF16(commandLine)));
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    CHECK(invocation.GetArgs() == originalValues);
    CHECK(args.GetArg(Execution::Args::Type::Word) == word);
    CHECK(args.GetArg(Execution::Args::Type::CommandLine) == nestedCommandLine);
    CHECK(args.GetArg(Execution::Args::Type::Position) == position);
    CHECK(args.GetCount(Execution::Args::Type::Proxy) == static_cast<size_t>(includeOuterProxy));
    if (includeOuterProxy)
    {
        CHECK(args.GetArg(Execution::Args::Type::Proxy) == outerProxy);
    }
    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    CHECK(logContents.find(expectedLog + "\r\n") != std::string::npos);
    CHECK(logContents.find("Command line Args: " + expectedCommandLine + "\r\n") != std::string::npos);
    if (expectedNestedCommandLine.find(proxyUri) == std::string::npos)
    {
        CHECK(logContents.find("ProxyUser") == std::string::npos);
        CHECK(logContents.find("ProxyPassword") == std::string::npos);
    }
}

TEST_CASE("Invocation_InvalidCompletionInputLogging", "[logging][complete]")
{
    using namespace AppInstaller::CLI;
    TestUserSettings userSettings;
    GroupPolicyTestOverride policies;
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();
    Invocation invocation{ { "complete", "--word", "--he", "--commandline", "winget install --he", "--position", "invalid" } };
    RootCommand root;
    const auto command = root.FindSubCommand(invocation);
    REQUIRE(command);
    Execution::Args args;
    REQUIRE_NOTHROW(command->ParseArguments(invocation, args));
    const std::wstring commandLine = L"winget complete --word --he --commandline \"winget install --he\" --position invalid";

    TempFile logFile{ "Invocation_InvalidCompletionInput", ".log" };
    Log().SetEnabledChannels(AppInstaller::Logging::Channel::CLI | AppInstaller::Logging::Channel::Core);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    REQUIRE_NOTHROW(invocation.Log(args, commandLine));
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    CHECK(logContents.find("WinGet invoked with arguments: 'complete' '--word' '--he' '--commandline' 'winget install --he' '--position' 'invalid'\r\n") != std::string::npos);
    CHECK(logContents.find("Command line Args: " + ConvertToUTF8(commandLine) + "\r\n") != std::string::npos);
}

TEST_CASE("Telemetry_StartupCommandLineLogging", "[logging]")
{
    const bool logCommandLine = GENERATE(false, true);
    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::TelemetryDisable>(true);
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();
    TempFile logFile{ "Telemetry_StartupCommandLine", ".log" };
    Log().SetEnabledChannels(DefaultChannel);
    Log().SetLevel(DefaultLevel);
    FileLogger::Add(logFile.GetPath());
    if (logCommandLine)
    {
        Telemetry().LogStartup();
    }
    else
    {
        Telemetry().LogStartup(false, false);
    }
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find("WinGet, version [") != std::string::npos);
    REQUIRE((logContents.find("Command line Args: ") != std::string::npos) == logCommandLine);
    if (logCommandLine)
    {
        REQUIRE(logContents.find("Command line Args: " + ConvertToUTF8(GetCommandLineW()) + "\r\n") != std::string::npos);
    }
}
