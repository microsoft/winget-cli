// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "..\WorkflowCommon.h"
#include <winget/AdminSettings.h>
#include <wil/win32_helpers.h>
#include <ManifestProviders/LocalManifestProvider.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Settings;

TEST_CASE("LocalManifestProvider_Load", "[LocalManifests][workflow]")
{
    REQUIRE(DisableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    auto path = GENERATE("Manifest-Good.yaml", "MultiFileManifestV1");
    auto argument = TestDataFile{ path }.GetPath().u8string();
    context.Args.AddArg(Args::Type::Manifest, argument);
    LocalManifestProvider provider{ argument };
    auto manifest = provider.Load(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(manifest);
    REQUIRE(manifest->Id == "microsoft.msixsdk");
}

TEST_CASE("LocalManifestProvider_MissingPath", "[LocalManifests][workflow]")
{
    TempFile missing{ "missing-manifest", ".yaml" };
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    auto argument = missing.GetPath().u8string();
    context.Args.AddArg(Args::Type::Manifest, argument);
    LocalManifestProvider provider{ argument };
    REQUIRE_FALSE(provider.Load(context));
    REQUIRE(context.IsTerminated());
}

TEST_CASE("LocalManifestProvider_LoadArchive", "[LocalManifests][workflow]")
{
    REQUIRE(DisableAdminSetting(BoolAdminSetting::RemoteManifestFiles));
    auto fixture = GENERATE("WrappedManifest.zip", "ManifestSource.zip");
    auto archive = std::filesystem::path{ wil::GetModuleFileNameW().get() }.parent_path() / fixture;
    TempFile input{ "local-manifest", ".ZIP" };
    std::filesystem::copy_file(archive, input.GetPath());
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    auto argument = input.GetPath().u8string();
    context.Args.AddArg(Args::Type::Manifest, argument);
    LocalManifestProvider provider{ argument };
    auto manifest = provider.Load(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(manifest);
    REQUIRE(manifest->Id == "Test.Package");
    REQUIRE(manifest->Version == "1.0.0");
    REQUIRE(std::filesystem::exists(input.GetPath()));
}

TEST_CASE("LocalManifestProvider_RejectsInvalidArchives", "[LocalManifests][workflow]")
{
    auto fixture = GENERATE("MultipleManifests.zip", "TestZip.zip", "invalid");
    TempFile input{ "invalid-manifest", ".zip" };
    if (fixture == "invalid"sv)
    {
        std::ofstream{ input.GetPath() } << "not a ZIP archive";
    }
    else
    {
        auto archive = std::filesystem::path{ wil::GetModuleFileNameW().get() }.parent_path() / fixture;
        std::filesystem::copy_file(archive, input.GetPath());
    }
    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto globals = context.SetForCurrentThread();
    bool relative = GENERATE(false, true);
    auto argument = (relative ? std::filesystem::relative(input.GetPath()) : input.GetPath()).u8string();
    context.Args.AddArg(Args::Type::Manifest, argument);
    LocalManifestProvider provider{ argument };
    REQUIRE_THROWS(provider.Load(context));
    REQUIRE(std::filesystem::exists(input.GetPath()));
}
