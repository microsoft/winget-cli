// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "..\WorkflowCommon.h"
#include "..\TestSettings.h"
#include <Commands/InstallCommand.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Settings;

TEST_CASE("ManifestProvider_IndependentAdminSettings", "[remoteManifest][LocalManifests][workflow]")
{
    bool localEnabled = GENERATE(false, true);
    bool remoteEnabled = GENERATE(false, true);
    bool remote = GENERATE(false, true);
    REQUIRE((localEnabled ? EnableAdminSetting(BoolAdminSetting::LocalManifestFiles) :
        DisableAdminSetting(BoolAdminSetting::LocalManifestFiles)));
    REQUIRE((remoteEnabled ? EnableAdminSetting(BoolAdminSetting::RemoteManifestFiles) :
        DisableAdminSetting(BoolAdminSetting::RemoteManifestFiles)));

    Execution::Args args;
    args.AddArg(Execution::Args::Type::Manifest, remote ? "https://example.com/package.yaml" :
        TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
    InstallCommand command({});
    if (remote ? remoteEnabled : localEnabled)
    {
        REQUIRE_NOTHROW(command.ValidateArguments(args));
    }
    else
    {
        REQUIRE_THROWS(command.ValidateArguments(args));
    }

    GroupPolicyTestOverride policies;
    policies.SetState(TogglePolicy::Policy::LocalManifestFiles, PolicyState::Disabled);
    REQUIRE_POLICY_EXCEPTION(command.ValidateArguments(args), TogglePolicy::Policy::LocalManifestFiles);
}
