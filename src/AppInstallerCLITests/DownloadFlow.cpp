// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include "AppInstallerRuntime.h"
#include "WorkflowCommon.h"
#include <Commands/DownloadCommand.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Authentication;
using namespace AppInstaller::CLI;

TEST_CASE("DownloadFlow_DownloadCommandProhibited", "[DownloadFlow][workflow]")
{
    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_DownloadCommandProhibited.yaml").GetPath().u8string());

    DownloadCommand download({});
    download.Execute(context);
    INFO(downloadOutput.str());

    // Verify AppInfo is printed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_DOWNLOAD_COMMAND_PROHIBITED);
    REQUIRE(downloadOutput.str().find(CLI::Resource::LocString(CLI::Resource::String::InstallerDownloadCommandProhibited).get()) != std::string::npos);
}

AppInstaller::Utility::DownloadResult ValidateAzureBlobStorageAuthHeaders(
    const std::string&,
    const std::filesystem::path& dest,
    AppInstaller::Utility::DownloadType,
    AppInstaller::IProgressCallback&,
    std::optional<AppInstaller::Utility::DownloadInfo> info)
{
    REQUIRE(info);
    REQUIRE(info->RequestHeaders.size() > 0);
    REQUIRE(info->RequestHeaders[0].IsAuth);
    REQUIRE(info->RequestHeaders[0].Name == "Authorization");
    REQUIRE(info->RequestHeaders[0].Value == "Bearer TestToken");
    REQUIRE_FALSE(info->RequestHeaders[1].IsAuth);
    REQUIRE(info->RequestHeaders[1].Name == "x-ms-version");
    // Not validating x-ms-version value

    std::ofstream file(dest, std::ofstream::out);
    file << "test";
    file.close();

    AppInstaller::Utility::DownloadResult result;
    result.Sha256Hash = AppInstaller::Utility::SHA256::ConvertToBytes("65DB2F2AC2686C7F2FD69D4A4C6683B888DC55BFA20A0E32CA9F838B51689A3B");
    return result;
}

TEST_CASE("DownloadFlow_DownloadWithInstallerAuthenticationSuccess", "[DownloadFlow][workflow]")
{
    if (Runtime::IsRunningAsSystem())
    {
        WARN("Test does not support running as system. Skipped.");
        return;
    }

    // Set authentication success result override
    std::string expectedToken = "TestToken";
    AuthenticationResult authResultOverride;
    authResultOverride.Status = S_OK;
    authResultOverride.Token = expectedToken;
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    // Set auth header validation override
    TestHook::SetDownloadResult_Function_Override downloadFunctionOverride({ &ValidateAzureBlobStorageAuthHeaders });

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("ManifestV1_10-InstallerAuthentication.yaml").GetPath().u8string());
    TestCommon::TempDirectory tempDirectory("TempDownload");
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory.GetPath().u8string());

    DownloadCommand download({});
    download.Execute(context);
    INFO(downloadOutput.str());

    // Verify success
    REQUIRE_FALSE(context.IsTerminated());
    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("DownloadFlow_DownloadWithInstallerAuthenticationNotSupported", "[DownloadFlow][workflow]")
{
    if (Runtime::IsRunningAsSystem())
    {
        WARN("Test does not support running as system. Skipped.");
        return;
    }

    // Set authentication failed result
    AuthenticationResult authResultOverride;
    authResultOverride.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED;
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("ManifestV1_10-InstallerAuthentication.yaml").GetPath().u8string());

    DownloadCommand download({});
    download.Execute(context);
    INFO(downloadOutput.str());

    // Verify AppInfo is printed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_AUTHENTICATION_TYPE_NOT_SUPPORTED);
    REQUIRE(downloadOutput.str().find(CLI::Resource::LocString(CLI::Resource::String::InstallerDownloadAuthenticationNotSupported).get()) != std::string::npos);
}

TEST_CASE("DownloadFlow_DownloadWithInstallerAuthenticationFailed", "[DownloadFlow][workflow]")
{
    if (Runtime::IsRunningAsSystem())
    {
        WARN("Test does not support running as system. Skipped.");
        return;
    }

    // Set authentication failed result
    AuthenticationResult authResultOverride;
    authResultOverride.Status = APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED;
    TestHook::SetAuthenticationResult_Override setAuthenticationResultOverride(authResultOverride);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("ManifestV1_10-InstallerAuthentication.yaml").GetPath().u8string());

    DownloadCommand download({});
    download.Execute(context);
    INFO(downloadOutput.str());

    // Verify AppInfo is printed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_AUTHENTICATION_FAILED);
    REQUIRE(downloadOutput.str().find(CLI::Resource::LocString(CLI::Resource::String::InstallerDownloadAuthenticationFailed).get()) != std::string::npos);
}
