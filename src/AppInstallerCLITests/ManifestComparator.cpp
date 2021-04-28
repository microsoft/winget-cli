// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Workflows/ManifestComparator.h>
#include <winget/UserSettings.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;

using Manifest = ::AppInstaller::Manifest::Manifest;

const ManifestInstaller& AddInstaller(Manifest& manifest, Architecture architecture, InstallerTypeEnum installerType, ScopeEnum scope = ScopeEnum::Unknown, std::string minOSVersion = {})
{
    ManifestInstaller toAdd;
    toAdd.Arch = architecture;
    toAdd.InstallerType = installerType;
    toAdd.Scope = scope;
    toAdd.MinOSVersion = minOSVersion;

    manifest.Installers.emplace_back(std::move(toAdd));

    return manifest.Installers.back();
}

void RequireInstaller(const std::optional<ManifestInstaller>& actual, const ManifestInstaller& expected)
{
    REQUIRE(actual);
    REQUIRE(actual->Arch == expected.Arch);
    REQUIRE(actual->InstallerType == expected.InstallerType);
    REQUIRE(actual->Scope == expected.Scope);
    REQUIRE(actual->MinOSVersion == expected.MinOSVersion);
}

TEST_CASE("ManifestComparator_OSFilter_Low", "[manifest_comparator]")
{
    Manifest manifest;
    AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::Unknown, "10.0.99999.0");

    ManifestComparator mc({}, {});
    auto result = mc.GetPreferredInstaller(manifest);

    REQUIRE(!result);
}

TEST_CASE("ManifestComparator_OSFilter_High", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller expected = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::Unknown, "10.0.0.0");

    ManifestComparator mc({}, {});
    auto result = mc.GetPreferredInstaller(manifest);

    RequireInstaller(result, expected);
}

TEST_CASE("ManifestComparator_InstalledScopeFilter_Uknown", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller unknown = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::Unknown);

    SECTION("Nothing Installed")
    {
        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, unknown);
    }
    SECTION("User Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::User);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
    }
    SECTION("Machine Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::Machine);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
    }
}

TEST_CASE("ManifestComparator_InstalledScopeFilter", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller user = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User);
    ManifestInstaller machine = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::Machine);

    SECTION("Nothing Installed")
    {
        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, user);
    }
    SECTION("User Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::User);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
    }
    SECTION("Machine Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::Machine);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
    }
}

TEST_CASE("ManifestComparator_InstalledTypeFilter", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller msi = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi);
    ManifestInstaller msix = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msix);

    SECTION("Nothing Installed")
    {
        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, msi);
    }
    SECTION("MSI Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Msi);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msi);
    }
    SECTION("MSIX Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Msix);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msix);
    }
}

TEST_CASE("ManifestComparator_InstalledTypeCompare", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller burn = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Burn);
    ManifestInstaller exe = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe);

    SECTION("Nothing Installed")
    {
        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, burn);
    }
    SECTION("Exe Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Exe);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, exe);
    }
    SECTION("Inno Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Inno);

        ManifestComparator mc({}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, burn);
    }
}

TEST_CASE("ManifestComparator_ScopeFilter", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller user = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User);
    ManifestInstaller machine = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::Machine);

    SECTION("Nothing Required")
    {
        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, user);
    }
    SECTION("User Required")
    {
        Args args;
        args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::User));

        ManifestComparator mc(args, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
    }
    SECTION("Machine Required")
    {
        Args args;
        args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::Machine));

        ManifestComparator mc(args, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
    }
}

TEST_CASE("ManifestComparator_ScopeCompare", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller machine = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::Machine);
    ManifestInstaller user = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User);

    SECTION("No Preference")
    {
        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // The default preference is user
        RequireInstaller(result, user);
    }
    SECTION("User Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallScopePreference>(ScopePreference::User);

        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
    }
    SECTION("Machine Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallScopePreference>(ScopePreference::Machine);

        ManifestComparator mc({}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
    }
}
