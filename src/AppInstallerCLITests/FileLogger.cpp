// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "WorkflowCommon.h"
#include <AppInstallerDownloader.h>
#include <AppInstallerFileLogger.h>
#include <AppInstallerStrings.h>
#include <AppInstallerTelemetry.h>
#include <COMContext.h>
#include <Commands/DscAdminSettingsResource.h>
#include <Commands/RootCommand.h>
#include <winget/ThreadGlobals.h>
#include <winget/NetworkSettings.h>
#include <winget/AdminSettings.h>
#include <winget/HttpClientHelper.h>
#include <future>
#include <set>
#include <tuple>
#include <wininet.h>

using namespace AppInstaller::Logging;
using namespace AppInstaller::Utility;
using namespace TestCommon;


std::string GetHeaderString()
{
    return "TIME [CHAN] Header Message";
}

std::string GetLargeString()
{
    return "[===|Clearly defined start to large string|===]\r\n"
        "While this string does not need to be particularly unique, it is still good if it is not easily duplicated by any other random set of data.\r\n"
        "It should also end in a character that is not used in any other way within these tests, so please don't include that character when writing tests.\r\n"
        "That character is &";
}

namespace
{
#define WINGET_DEFINE_STRING_ENUM(_enum_,_value_) constexpr std::string_view _enum_##_##_value_ = #_value_##sv

    WINGET_DEFINE_STRING_ENUM(TagState, Unset);
    WINGET_DEFINE_STRING_ENUM(TagState, SetAtStart);
    WINGET_DEFINE_STRING_ENUM(TagState, SetAfterLogging);

    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, Zero);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, SmallerThanLargeString);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, EqualToLargeString);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, SlightlyLargerThanLargeString);
    WINGET_DEFINE_STRING_ENUM(MaximumSizeState, MuchLargerThanLargeString);

    constexpr std::string_view WrapIndicator = "--- log file has wrapped ---"sv;
    // The amount of extra size that is allowed (total indicator size + newline + newlines from test strings)
    constexpr size_t ExtraAllowedSize = 72;

    constexpr size_t NewLineCharacterCount = 2;
    constexpr size_t SmallDifferenceSize = 10;
    constexpr AppInstaller::Logging::Channel DefaultChannel = AppInstaller::Logging::Channel::Core;
    constexpr AppInstaller::Logging::Level DefaultLevel = AppInstaller::Logging::Level::Info;

    void ValidateFileContents(const std::filesystem::path& file, const std::vector<std::string_view>& expectedContents, size_t maximumSize)
    {
        std::ifstream fileStream{ file, std::ios::binary };
        auto fileContents = ReadEntireStream(fileStream);
        std::string_view fileContentsView = fileContents;

        std::string fileContentsCopy = fileContents;
        FindAndReplace(fileContentsCopy, "\r", "\\r");
        FindAndReplace(fileContentsCopy, "\n", "\\n");
        INFO("File contents:\n" << fileContentsCopy);

        if (maximumSize)
        {
            REQUIRE(maximumSize + ExtraAllowedSize >= fileContents.size());
        }

        size_t currentPosition = 0;
        for (std::string_view expectedContent : expectedContents)
        {
            REQUIRE(currentPosition < fileContents.size());

            if (expectedContent == WrapIndicator)
            {
                auto endLinePosition = fileContentsView.find('\n', currentPosition);
                REQUIRE(endLinePosition != -1);
                REQUIRE(endLinePosition >= expectedContent.size() + NewLineCharacterCount);
                auto actualContent = fileContentsView.substr(endLinePosition + 1 - expectedContent.size() - NewLineCharacterCount, expectedContent.size());
                REQUIRE(expectedContent == actualContent);
                currentPosition = endLinePosition + 1;
            }
            else
            {
                auto actualContent = fileContentsView.substr(currentPosition, expectedContent.size());
                REQUIRE(expectedContent == actualContent);
                currentPosition += expectedContent.size() + NewLineCharacterCount;
            }
        }
    }

    void FileLogger_MaximumSize_Test(std::string_view tagState, std::string_view sizeState)
    {
        auto headerString = GetHeaderString();
        auto largeString = GetLargeString();

        // Determine maximum size
        size_t maximumSize = 0;

        if (sizeState == MaximumSizeState_SmallerThanLargeString)
        {
            maximumSize = largeString.size() - SmallDifferenceSize;
        }
        else if (sizeState == MaximumSizeState_EqualToLargeString)
        {
            maximumSize = largeString.size();
        }
        else if (sizeState == MaximumSizeState_SlightlyLargerThanLargeString)
        {
            maximumSize = largeString.size() + SmallDifferenceSize;
        }
        else if (sizeState == MaximumSizeState_MuchLargerThanLargeString)
        {
            maximumSize = largeString.size() * 2;
        }

        INFO("Tag State: " << tagState << ", Size State: " << sizeState << "[" << maximumSize << "]");

        TempFile tempFile{ "FileLogger_MaximumSize", ".log" };
        FileLogger logger{ tempFile };

        INFO("File: " << tempFile.GetPath().u8string());

        logger.SetMaximumSize(wil::safe_cast<std::ofstream::off_type>(maximumSize));

        // Set tag and log strings
        size_t tagPosition = 0;
        if (tagState == TagState_SetAtStart)
        {
            logger.SetTag(Tag::HeadersComplete);
        }

        logger.WriteDirect(DefaultChannel, DefaultLevel, headerString);

        if (tagState == TagState_SetAfterLogging)
        {
            logger.SetTag(Tag::HeadersComplete);
            tagPosition = headerString.size() + NewLineCharacterCount;
        }

        // Due to text output in the logger, log with \n only
        std::string largeStringWithoutCarriageReturn = largeString;
        FindAndReplace(largeStringWithoutCarriageReturn, "\r\n", "\n");

        logger.WriteDirect(DefaultChannel, DefaultLevel, largeStringWithoutCarriageReturn);

        // Calculate current state
        size_t maximumAvailableSpace = std::numeric_limits<size_t>::max();
        size_t currentAvailableSpace = std::numeric_limits<size_t>::max();
        if (maximumSize)
        {
            maximumAvailableSpace = maximumSize - tagPosition;
            currentAvailableSpace = maximumSize - headerString.size() - NewLineCharacterCount;
        }

        bool shouldWrap = largeString.size() > currentAvailableSpace;

        INFO("Maximum Available: " << maximumAvailableSpace << ", Current Available: " << currentAvailableSpace << ", ShouldWrap: " << shouldWrap);

        std::vector<std::string_view> expectedFileContents;

        if (tagPosition || !shouldWrap)
        {
            expectedFileContents.push_back(headerString);
        }

        if (shouldWrap)
        {
            expectedFileContents.push_back(WrapIndicator);
        }

        std::string_view largeStringView = largeString;
        expectedFileContents.push_back(largeStringView.substr(0, std::min(largeString.size(), maximumAvailableSpace)));

        ValidateFileContents(tempFile, expectedFileContents, maximumSize);

        // Log again
        INFO("Second time logging large string");
        logger.WriteDirect(DefaultChannel, DefaultLevel, largeStringWithoutCarriageReturn);

        // The maximum size is twice the large log, so anything with a limit will wrap
        shouldWrap = maximumSize != 0;

        expectedFileContents.clear();

        if (tagPosition || !shouldWrap)
        {
            expectedFileContents.push_back(headerString);
        }

        if (shouldWrap)
        {
            expectedFileContents.push_back(WrapIndicator);
        }
        else
        {
            expectedFileContents.push_back(largeStringView);
        }

        expectedFileContents.push_back(largeStringView.substr(0, std::min(largeString.size(), maximumAvailableSpace)));

        ValidateFileContents(tempFile, expectedFileContents, maximumSize);
    }
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

TEST_CASE("Invocation_StartupLogsSurviveWrapping", "[logging]")
{
    using namespace AppInstaller::CLI;
    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::TelemetryDisable>(true);
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();
    const std::string proxy = "https://ProxyUser:ProxyPassword@proxy.invalid:8080";
    Invocation invocation{ { "install", "--proxy", proxy } };
    Execution::Args args;
    args.AddArg(Execution::Args::Type::Proxy, proxy);

    TempFile logFile{ "Invocation_StartupLogsWrapping", ".log" };
    auto logger = std::make_unique<FileLogger>(logFile.GetPath());
    auto* fileLogger = logger.get();
    Log().SetEnabledChannels(AppInstaller::Logging::Channel::CLI | AppInstaller::Logging::Channel::Core);
    Log().SetLevel(DefaultLevel);
    Log().AddLogger(std::move(logger));
    Telemetry().LogStartup(false, false);
    invocation.Log(args, ConvertToUTF16("winget install --proxy " + proxy));
    const auto headerSize = std::filesystem::file_size(logFile.GetPath());
    fileLogger->SetMaximumSize(static_cast<std::ofstream::off_type>(headerSize + 32));
    fileLogger->WriteDirect(DefaultChannel, DefaultLevel, std::string(static_cast<size_t>(headerSize + 64), 'x'));
    Log().RemoveLogger(FileLogger::GetNameForPath(logFile.GetPath()));

    std::ifstream logStream{ logFile.GetPath(), std::ios::binary };
    const auto logContents = ReadEntireStream(logStream);
    INFO("Log contents:\n" << logContents);
    REQUIRE(logContents.find(WrapIndicator) != std::string::npos);
    CHECK(logContents.find("WinGet, version [") != std::string::npos);
    CHECK(logContents.find("Command line Args: winget install --proxy https://[redacted]@proxy.invalid:8080/\r\n") != std::string::npos);
    CHECK(logContents.find("WinGet invoked with arguments: 'install' '--proxy' 'https://[redacted]@proxy.invalid:8080/'\r\n") != std::string::npos);
    CHECK(logContents.find("ProxyUser") == std::string::npos);
    CHECK(logContents.find("ProxyPassword") == std::string::npos);
}

TEST_CASE("COMFileLogging_InitializationFailureIsNonFatal", "[logging]")
{
    TestUserSettings settings;
    AppInstaller::ThreadLocalStorage::WingetThreadGlobals threadGlobals;
    auto previousThreadGlobals = threadGlobals.SetForCurrentThread();

    TempFile blocker{ "FileLogger_BlockedParent", ".tmp" };
    {
        std::ofstream file{ blocker.GetPath() };
        file << "Not a directory";
        REQUIRE(file.good());
    }

    auto logPath = blocker.GetPath() / "test.log";
    REQUIRE_THROWS(FileLogger{ logPath });
    REQUIRE_THROWS(FileLogger::Add(logPath));
    REQUIRE_FALSE(Log().ContainsLogger(FileLogger::GetNameForPath(logPath)));

    using namespace AppInstaller::Runtime;
    auto restorePaths = wil::scope_exit([]()
        {
            TestHook_ClearPathOverrides();
            SetTestPathOverrides();
        });
    AppInstaller::Filesystem::PathDetails blockedPath;
    blockedPath.Path = blocker.GetPath() / "logs";
    blockedPath.Create = GENERATE(false, true);
    TestHook_SetPathOverride(PathName::DefaultLogLocation, blockedPath);

    REQUIRE_NOTHROW(AppInstaller::CLI::Execution::COMContext::SetLoggers(AppInstaller::Logging::Channel::All, Level::Verbose));
    REQUIRE_FALSE(Log().ContainsLogger("file"));
    REQUIRE(Log().ContainsLogger("Trace"));
    REQUIRE_FALSE(Log().ContainsLogger("OutputDebugStringLogger"));
}

TEST_CASE("FileLogger_MaximumSize", "[logging]")
{
    auto tagState = GENERATE(TagState_Unset, TagState_SetAtStart, TagState_SetAfterLogging);
    auto sizeState = GENERATE(MaximumSizeState_Zero, MaximumSizeState_SmallerThanLargeString, MaximumSizeState_EqualToLargeString, MaximumSizeState_SlightlyLargerThanLargeString, MaximumSizeState_MuchLargerThanLargeString);
    FileLogger_MaximumSize_Test(tagState, sizeState);
}

TEST_CASE("FileLogger_MaximumSize_ManyWraps", "[logging]")
{
    TempFile tempFile{ "FileLogger_ManyWraps", ".log" };
    FileLogger logger{ tempFile };

    INFO("File: " << tempFile.GetPath().u8string());

    size_t maximumSize = 1000;
    logger.SetMaximumSize(static_cast<std::ofstream::off_type>(maximumSize));

    std::string header = GetHeaderString();
    header += " !Now with more header!";
    std::string largeString = "[*=INIT=*]Now we just need another few dozen characters, which shouldn't be that hard to get. Wow, made it already.";
    std::string_view largeStringView = largeString;
    size_t initSize = 10;

    logger.WriteDirect(DefaultChannel, DefaultLevel, header);
    logger.SetTag(Tag::HeadersComplete);

    // Use the default seed value as we want arbitrary but reproducible results
    std::default_random_engine randomEngine;
    std::uniform_int_distribution<> sizeDistribution(static_cast<int>(initSize), 100);

    // We should expect ~500 wraps on average
    for (size_t i = 0; i < 9999; ++i)
    {
        logger.WriteDirect(DefaultChannel, DefaultLevel, largeStringView.substr(0, sizeDistribution(randomEngine)));
    }

    // We want the header to be preserved, followed by the wrap indicator, and at a minimum we should see the first few characters in the log string
    std::vector<std::string_view> expectedFileContents;
    expectedFileContents.push_back(header);
    expectedFileContents.push_back(WrapIndicator);
    expectedFileContents.push_back(largeStringView.substr(0, initSize));

    ValidateFileContents(tempFile, expectedFileContents, maximumSize);
}

TEST_CASE("FileLogger_ConcurrentWritesPreserveCompleteLines", "[logging]")
{
    bool formatted = GENERATE(false, true);
    std::ofstream::off_type maximumSize = GENERATE(0, 4 * 1024 * 1024);
    TempFile tempFile{ "FileLogger_ConcurrentWrites", ".log" };
    constexpr size_t workerCount = 8;
    constexpr size_t messagesPerWorker = 1000;
    std::vector<std::vector<std::string>> messages(workerCount);
    std::set<std::string> expected;
    for (size_t worker = 0; worker < workerCount; ++worker)
    {
        for (size_t message = 0; message < messagesPerWorker; ++message)
        {
            auto text = "parallel-log:" + std::to_string(worker) + ":" + std::to_string(message) + ":" + std::string(128, 'x');
            expected.insert(text);
            messages[worker].emplace_back(std::move(text));
        }
    }

    {
        FileLogger original{ tempFile };
        original.SetMaximumSize(maximumSize);
        FileLogger logger{ std::move(original) };
        std::promise<void> start;
        auto started = start.get_future().share();
        std::vector<std::future<void>> workers;
        auto releaseOnFailure = wil::scope_exit([&]() { start.set_value(); });
        for (size_t worker = 0; worker < workerCount; ++worker)
        {
            workers.emplace_back(std::async(std::launch::async, [&, worker]()
                {
                    started.wait();
                    for (const auto& message : messages[worker])
                    {
                        if (formatted)
                        {
                            logger.Write(DefaultChannel, DefaultLevel, message);
                        }
                        else
                        {
                            logger.WriteDirect(DefaultChannel, DefaultLevel, message);
                        }
                    }
                }));
        }
        start.set_value();
        releaseOnFailure.release();
        for (auto& worker : workers)
        {
            worker.get();
        }
    }

    std::ifstream file{ tempFile.GetPath() };
    REQUIRE(file.is_open());
    std::string line;
    size_t lineCount = 0;
    while (std::getline(file, line))
    {
        INFO("Line " << lineCount << ": " << line);
        std::string message = line;
        if (formatted)
        {
            constexpr std::string_view prefix = " <I> [CORE] ";
            auto position = line.find(prefix);
            REQUIRE(position != std::string::npos);
            message = line.substr(position + prefix.size());
        }
        REQUIRE(expected.erase(message) == 1);
        ++lineCount;
    }
    REQUIRE(file.eof());
    REQUIRE(lineCount == workerCount * messagesPerWorker);
    REQUIRE(expected.empty());
}
