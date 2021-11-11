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

void RequireInapplicabilities(const std::vector<InapplicabilityFlags>& inapplicabilities, const std::vector<InapplicabilityFlags>& expected)
{
    REQUIRE(inapplicabilities.size() == expected.size());

    for (std::size_t  i = 0; i < inapplicabilities.size(); i++)
    {
        REQUIRE(inapplicabilities[i] == expected[i]);
    }
}

TEST_CASE("ManifestComparator_OSFilter_Low", "[manifest_comparator]")
{
    Manifest manifest;
    AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::Unknown, "10.0.99999.0");

    ManifestComparator mc(ManifestComparatorTestContext{}, {});
    auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

    REQUIRE(!result);
    RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::OSVersion });
}

TEST_CASE("ManifestComparator_OSFilter_High", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller expected = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::Unknown, "10.0.0.0");

    ManifestComparator mc(ManifestComparatorTestContext{}, {});
    auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

    RequireInstaller(result, expected);
    REQUIRE(inapplicabilities.size() == 0);
}

TEST_CASE("ManifestComparator_InstalledScopeFilter_Uknown", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller unknown = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::Unknown);

    SECTION("Nothing Installed")
    {
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, unknown);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("User Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::User);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Machine Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::Machine);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
        REQUIRE(inapplicabilities.size() == 0);
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, user);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("User Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::User);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledScope });
    }
    SECTION("Machine Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledScope] = ScopeToString(ScopeEnum::Machine);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledScope });
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, msi);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("MSI Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Msi);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msi);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledType });
    }
    SECTION("MSIX Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Msix);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msix);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledType });
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, burn);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Exe Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Exe);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, exe);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Inno Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Inno);

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, burn);
        REQUIRE(inapplicabilities.size() == 0);
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, user);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("User Required")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::User));

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::Scope });
    }
    SECTION("Machine Required")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::Machine));

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::Scope });
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // The default preference is user
        RequireInstaller(result, user);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("User Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallScopePreference>(ScopePreference::User);

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Machine Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallScopePreference>(ScopePreference::Machine);

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
        REQUIRE(inapplicabilities.size() == 0);
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, enGB);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("en-US Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "en-US";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("zh-CN Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "zh-CN";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledLocale });
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        // Only because it is first
        RequireInstaller(result, enGB);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("en-US Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "en-US";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledLocale });
    }
    SECTION("zh-CN Installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "zh-CN";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledLocale, InapplicabilityFlags::InstalledLocale });
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::Locale , InapplicabilityFlags::Locale });
    }
    SECTION("zh-CN Required")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::Locale, "zh-CN"s);

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::Locale , InapplicabilityFlags::Locale, InapplicabilityFlags::Locale });
    }
    SECTION("en-US Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallLocalePreference>({ "en-US" });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, enGB);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("zh-CN Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallLocalePreference>({ "zh-CN" });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, unknown);
        REQUIRE(inapplicabilities.size() == 0);
    }
}

TEST_CASE("ManifestComparator_AllowedArchitecture_x64_only", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller x86 = AddInstaller(manifest, Architecture::X86, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");

    ManifestComparatorTestContext context;
    context.Add<Data::AllowedArchitectures>({ Architecture::X64 });

    ManifestComparator mc(context, {});
    auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

    REQUIRE(!result);
    RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::MachineArchitecture });
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
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, x86);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::MachineArchitecture, InapplicabilityFlags::MachineArchitecture });
    }
    SECTION("Unknown")
    {
        ManifestComparatorTestContext context;
        context.Add<Data::AllowedArchitectures>({ Architecture::Unknown });

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(result);
        REQUIRE(result->Arch == GetApplicableArchitectures()[0]);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::MachineArchitecture, InapplicabilityFlags::MachineArchitecture });
    }
    SECTION("x86 and Unknown")
    {
        ManifestComparatorTestContext context;
        context.Add<Data::AllowedArchitectures>({ Architecture::X86, Architecture::Unknown });

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, x86);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::MachineArchitecture, InapplicabilityFlags::MachineArchitecture });
    }
}

TEST_CASE("ManifestComparator_Inapplicabilities", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller installer = AddInstaller(manifest, Architecture::Arm64, InstallerTypeEnum::Exe, ScopeEnum::User, "10.0.99999.0", "es-MX");

    ManifestComparatorTestContext context;
    context.Add<Data::AllowedArchitectures>({ Architecture::X86 });
    context.Args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::Machine));
    context.Args.AddArg(Args::Type::Locale, "en-GB"s);

    IPackageVersion::Metadata metadata;
    metadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(InstallerTypeEnum::Msix);

    ManifestComparator mc(context, metadata);
    auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

    REQUIRE(!result);
    RequireInapplicabilities(
        inapplicabilities,
        { InapplicabilityFlags::OSVersion | InapplicabilityFlags::InstalledType | InapplicabilityFlags::Locale | InapplicabilityFlags::Scope | InapplicabilityFlags::MachineArchitecture });
}
