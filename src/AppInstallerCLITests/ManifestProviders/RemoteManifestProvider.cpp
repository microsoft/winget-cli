// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "..\WorkflowCommon.h"
#include "..\TestHooks.h"
#include <winget/AdminSettings.h>
#include <wil/win32_helpers.h>
#include <AppInstallerDownloader.h>
#include <ManifestProviders/RemoteManifestProvider.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

TEST_CASE("RemoteManifestProvider_Load", "[remoteManifest][workflow]")
{
    REQUIRE(EnableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    auto url = GENERATE("https://example.com/package.yaml"sv, "https://example.com/package.yml"sv,
        "https://example.com/manifests.zip"sv, "https://example.com/wrapped.zip"sv);
    std::filesystem::path temporaryDirectory;
    TestHook::SetDownloadResult_Function_Override download{
        [&](const std::string& downloadUrl, const std::filesystem::path& destination, DownloadType type,
            AppInstaller::IProgressCallback&, std::optional<DownloadInfo>)
        {
            REQUIRE(downloadUrl == url);
            REQUIRE(type == DownloadType::Manifest);
            temporaryDirectory = destination.parent_path();
            auto fixture = url.substr(url.size() - 4) == ".zip" ?
                std::filesystem::path{ wil::GetModuleFileNameW().get() }.parent_path() /
                    (url == "https://example.com/wrapped.zip"sv ? "WrappedManifest.zip" : "ManifestSource.zip") :
                TestDataFile{ "Manifest-Good.yaml" }.GetPath();
            std::filesystem::copy_file(fixture, destination);
            return DownloadResult{};
        } };
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    RemoteManifestProvider provider{ url };
    auto manifest = provider.Load(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(manifest);
    bool archive = url.substr(url.size() - 4) == ".zip";
    REQUIRE(manifest->Id == (archive ? "Test.Package" : "microsoft.msixsdk"));
    REQUIRE(manifest->Version == (archive ? "1.0.0" : "1.7.32"));
    REQUIRE_FALSE(std::filesystem::exists(temporaryDirectory));
}

TEST_CASE("RemoteManifestProvider_RejectsUrlsBeforeDownload", "[remoteManifest][workflow]")
{
    REQUIRE(EnableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    auto url = GENERATE("http://example.com/package.yaml"sv, "https://user@example.com/package.yaml"sv,
        "https://example.com/package.yaml?token=secret"sv, "https://example.com/package.zip#fragment"sv,
        "https://example.com:8443/package.yaml"sv, "https://example.com/package.txt"sv);
    bool downloaded = false;
    TestHook::SetDownloadResult_Function_Override download{
        [&](const std::string&, const std::filesystem::path&, DownloadType,
            AppInstaller::IProgressCallback&, std::optional<DownloadInfo>)
        {
            downloaded = true;
            return DownloadResult{};
        } };
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    RemoteManifestProvider provider{ url };
    REQUIRE_THROWS(provider.Load(context));
    REQUIRE_FALSE(downloaded);
}

TEST_CASE("RemoteManifestProvider_InvalidDownloadCleanup", "[remoteManifest][workflow]")
{
    REQUIRE(EnableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    auto extension = GENERATE(std::string{ ".zip" }, std::string{ ".yaml" });
    std::filesystem::path temporaryDirectory;
    TestHook::SetDownloadResult_Function_Override download{
        [&](const std::string&, const std::filesystem::path& destination, DownloadType,
            AppInstaller::IProgressCallback&, std::optional<DownloadInfo>)
        {
            temporaryDirectory = destination.parent_path();
            std::ofstream{ destination } << "invalid: [yaml";
            return DownloadResult{};
        } };
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    RemoteManifestProvider provider{ "https://example.com/package" + extension };
    REQUIRE_THROWS(provider.Load(context));
    REQUIRE_FALSE(std::filesystem::exists(temporaryDirectory));
}

TEST_CASE("RemoteManifestProvider_DisabledBeforeDownload", "[remoteManifest][workflow]")
{
    REQUIRE(DisableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    bool downloaded = false;
    TestHook::SetDownloadResult_Function_Override download{
        [&](const std::string&, const std::filesystem::path&, DownloadType,
            AppInstaller::IProgressCallback&, std::optional<DownloadInfo>)
        {
            downloaded = true;
            return DownloadResult{};
        } };
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    RemoteManifestProvider provider{ "https://example.com/package.yaml" };
    REQUIRE_FALSE(provider.Load(context));
    REQUIRE(context.GetTerminationHR() == E_ACCESSDENIED);
    REQUIRE_FALSE(downloaded);
}

TEST_CASE("RemoteManifestProvider_DownloadFailureCleanup", "[remoteManifest][workflow]")
{
    REQUIRE(EnableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    bool cancelled = GENERATE(false, true);
    std::filesystem::path temporaryDirectory;
    TestHook::SetDownloadResult_Function_Override download{
        [&](const std::string&, const std::filesystem::path& destination, DownloadType,
            AppInstaller::IProgressCallback&, std::optional<DownloadInfo>) -> DownloadResult
        {
            temporaryDirectory = destination.parent_path();
            THROW_HR(cancelled ? E_ABORT : E_FAIL);
        } };
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    RemoteManifestProvider provider{ "https://example.com/package.yaml" };
    REQUIRE_THROWS(provider.Load(context));
    REQUIRE_FALSE(std::filesystem::exists(temporaryDirectory));
}

TEST_CASE("RemoteManifestProvider_ArchiveRequiresOneManifest", "[remoteManifest][workflow]")
{
    REQUIRE(EnableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    auto fixture = GENERATE("MultipleManifests.zip", "TestZip.zip");
    std::filesystem::path temporaryDirectory;
    TestHook::SetDownloadResult_Function_Override download{
        [&](const std::string&, const std::filesystem::path& destination, DownloadType,
            AppInstaller::IProgressCallback&, std::optional<DownloadInfo>)
        {
            temporaryDirectory = destination.parent_path();
            auto archive = std::filesystem::path{ wil::GetModuleFileNameW().get() }.parent_path() / fixture;
            std::filesystem::copy_file(archive, destination);
            return DownloadResult{};
        } };
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    RemoteManifestProvider provider{ "https://example.com/package.zip" };
    REQUIRE_THROWS(provider.Load(context));
    REQUIRE_FALSE(std::filesystem::exists(temporaryDirectory));
}
