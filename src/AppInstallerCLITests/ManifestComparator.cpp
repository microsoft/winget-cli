// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <ExecutionContext.h>
#include <COMContext.h>
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

struct ManifestComparatorTestContext : public NullStream, Context
{
    ManifestComparatorTestContext() : NullStream(), Context(*m_nullOut, *m_nullIn) {}
};

const ManifestInstaller& AddInstaller(Manifest& manifest, Architecture architecture, InstallerTypeEnum installerType, ScopeEnum scope = ScopeEnum::Unknown, std::string minOSVersion = {}, std::string locale = {})
{
    ManifestInstaller toAdd;
    toAdd.Arch = architecture;
    toAdd.InstallerType = installerType;
    toAdd.Scope = scope;
    toAdd.MinOSVersion = minOSVersion;
    toAdd.Locale = locale;

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
    REQUIRE(actual->Locale == expected.Locale);
}

TEST_CASE("ManifestComparator_OSFilter_Low", "[manifest_comparator]")
{
    Manifest manifest;
    AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::Unknown, "10.0.99999.0");

    ManifestComparator mc(ManifestComparatorTestContext{}, {});
    auto result = mc.GetPreferredInstaller(manifest);

    REQUIRE(!result);
}

TEST_CASE("ManifestComparator_OSFilter_High", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller expected = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::Unknown, "10.0.0.0");

    ManifestComparator mc(ManifestComparatorTestContext{}, {});
    auto result = mc.GetPreferredInstaller(manifest);

    RequireInstaller(result, expected);
}

TEST_CASE("ManifestComparator_InstalledScopeFilter_Uknown", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller unknown = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::Unknown);

    SECTION("Nothing Installed")
    {
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, unknown);
    }
    SECTION("User Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::User);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
    }
    SECTION("Machine Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::Machine);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
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
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, user);
    }
    SECTION("User Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::User);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
    }
    SECTION("Machine Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::Machine);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
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
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, msi);
    }
    SECTION("MSI Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Msi);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msi);
    }
    SECTION("MSIX Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Msix);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
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
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, burn);
    }
    SECTION("Exe Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Exe);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, exe);
    }
    SECTION("Inno Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Inno);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
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
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, user);
    }
    SECTION("User Required")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::User));

        ManifestComparator mc(context, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
    }
    SECTION("Machine Required")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::Machine));

        ManifestComparator mc(context, {});
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
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // The default preference is user
        RequireInstaller(result, user);
    }
    SECTION("User Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallScopePreference>(ScopePreference::User);

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
    }
    SECTION("Machine Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallScopePreference>(ScopePreference::Machine);

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
    }
}

TEST_CASE("ManifestComparator_InstalledLocaleComparator_Uknown", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller unknown = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");
    ManifestInstaller enGB = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "en-GB");

    SECTION("Nothing Installed en-US preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallLocalePreference>({ "en-US" });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, enGB);
    }
    SECTION("en-US Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "en-US";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
    }
    SECTION("zh-CN Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "zh-CN";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
    }
}

TEST_CASE("ManifestComparator_InstalledLocaleComparator", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller frFR = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "fr-FR");
    ManifestInstaller enGB = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "en-GB");

    SECTION("Nothing Installed en-US preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallLocalePreference>({ "en-US" });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, enGB);
    }
    SECTION("en-US Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "en-US";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
    }
    SECTION("zh-CN Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "zh-CN";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto result = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
    }
}

TEST_CASE("ManifestComparator_LocaleComparator", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller unknown = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");
    ManifestInstaller frFR = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "fr-FR");
    ManifestInstaller enGB = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "en-GB");

    SECTION("en-GB Required")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::Locale, "en-GB"s);

        ManifestComparator mc(context, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
    }
    SECTION("zh-CN Required")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::Locale, "zh-CN"s);

        ManifestComparator mc(context, {});
        auto result = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
    }
    SECTION("en-US Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallLocalePreference>({ "en-US" });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
    }
    SECTION("zh-CN Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallLocalePreference>({ "zh-CN" });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
    }
}

TEST_CASE("ManifestComparator_AllowedArchitecture_x64_only", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller x86 = AddInstaller(manifest, Architecture::X86, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");

    ManifestComparatorTestContext context;
    context.Add<Data::AllowedArchitectures>({ Architecture::X64 });

    ManifestComparator mc(context, {});
    auto result = mc.GetPreferredInstaller(manifest);

    REQUIRE(!result);
}

TEST_CASE("ManifestComparator_AllowedArchitecture", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller x86 = AddInstaller(manifest, Architecture::X86, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");
    ManifestInstaller x64 = AddInstaller(manifest, Architecture::X64, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");
    ManifestInstaller arm = AddInstaller(manifest, Architecture::Arm, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");
    ManifestInstaller arm64 = AddInstaller(manifest, Architecture::Arm64, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");

    SECTION("x86 Preference")
    {
        ManifestComparatorTestContext context;
        context.Add<Data::AllowedArchitectures>({ Architecture::X86, Architecture::X64 });

        ManifestComparator mc(context, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, x86);
    }
    SECTION("Neutral")
    {
        ManifestComparatorTestContext context;
        context.Add<Data::AllowedArchitectures>({ Architecture::Neutral });

        ManifestComparator mc(context, {});
        auto result = mc.GetPreferredInstaller(manifest);

        REQUIRE(result);
        REQUIRE(result->Arch == GetApplicableArchitectures()[0]);
    }
    SECTION("x86 and Neutral")
    {
        ManifestComparatorTestContext context;
        context.Add<Data::AllowedArchitectures>({ Architecture::X86, Architecture::Neutral });

        ManifestComparator mc(context, {});
        auto result = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, x86);
    }
}
