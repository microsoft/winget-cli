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

const ManifestInstaller& AddInstaller(
    Manifest& manifest,
    Architecture architecture,
    InstallerTypeEnum installerType,
    ScopeEnum scope = ScopeEnum::Unknown,
    std::string minOSVersion = {},
    std::string locale = {},
    std::vector<Architecture> unsupportedOSArchitectures = {},
    MarketsInfo markets = {})
{
    ManifestInstaller toAdd;
    toAdd.Arch = architecture;
    toAdd.BaseInstallerType = installerType;
    toAdd.Scope = scope;
    toAdd.MinOSVersion = minOSVersion;
    toAdd.Locale = locale;
    toAdd.UnsupportedOSArchitectures = unsupportedOSArchitectures;
    toAdd.Markets = markets;

    manifest.Installers.emplace_back(std::move(toAdd));

    return manifest.Installers.back();
}

template<typename T>
void RequireVectorsEqual(const std::vector<T>& actual, const std::vector<T>& expected)
{
    REQUIRE(actual.size() == expected.size());
    
    for (std::size_t i = 0; i < actual.size(); ++i)
    {
        REQUIRE(actual[i] == expected[i]);
    }
}

void RequireInstaller(const std::optional<ManifestInstaller>& actual, const ManifestInstaller& expected)
{
    REQUIRE(actual);
    REQUIRE(actual->Arch == expected.Arch);
    REQUIRE(actual->EffectiveInstallerType() == expected.EffectiveInstallerType());
    REQUIRE(actual->Scope == expected.Scope);
    REQUIRE(actual->MinOSVersion == expected.MinOSVersion);
    REQUIRE(actual->Locale == expected.Locale);

    RequireVectorsEqual(actual->Markets.AllowedMarkets, expected.Markets.AllowedMarkets);
    RequireVectorsEqual(actual->Markets.ExcludedMarkets, expected.Markets.ExcludedMarkets);
}

void RequireInapplicabilities(const std::vector<InapplicabilityFlags>& inapplicabilities, const std::vector<InapplicabilityFlags>& expected)
{
    RequireVectorsEqual(inapplicabilities, expected);
}

void RequireInapplicabilityType(const std::vector<InapplicabilityFlags>& inapplicabilities, InapplicabilityFlags expected)
{
    REQUIRE(!inapplicabilities.empty());
    for (std::size_t i = 0; i < inapplicabilities.size(); ++i)
    {
        REQUIRE(inapplicabilities[i] == expected);
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

TEST_CASE("ManifestComparator_InstalledScopeFilter_Unknown", "[manifest_comparator]")
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

        RequireInstaller(result, burn);
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
        settings.Set<Setting::InstallScopePreference>(ScopeEnum::User);

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, user);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Machine Preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallScopePreference>(ScopeEnum::Machine);

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, machine);
        REQUIRE(inapplicabilities.size() == 0);
    }
}

TEST_CASE("ManifestComparator_LocaleComparator_Installed_WithUnknown", "[manifest_comparator]")
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

TEST_CASE("ManifestComparator_LocaleComparator_Installed", "[manifest_comparator]")
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
    SECTION("en-US installed but fr-fr as user intent")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "en-US";
        metadata[PackageVersionMetadata::UserIntentLocale] = "fr-FR";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, frFR);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstalledLocale }); // en-US inapplicable
    }
    SECTION("en-US installed but zh-CN as user intent")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledLocale] = "en-US";
        metadata[PackageVersionMetadata::UserIntentLocale] = "zh-CN";

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
        RequireInapplicabilityType(inapplicabilities, InapplicabilityFlags::MachineArchitecture);
    }
    SECTION("x86 and Unknown")
    {
        ManifestComparatorTestContext context;
        context.Add<Data::AllowedArchitectures>({ Architecture::X86, Architecture::Unknown });

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, x86);
        RequireInapplicabilityType(inapplicabilities, InapplicabilityFlags::MachineArchitecture);
    }
}

TEST_CASE("ManifestComparator_Architectures_WithUserIntent", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller x86 = AddInstaller(manifest, Architecture::X86, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");
    ManifestInstaller x64 = AddInstaller(manifest, Architecture::X64, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");

    SECTION("x86 installed")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledArchitecture] = "x86";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, x86);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("x86 installed but x64 as user intent")
    {
        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledArchitecture] = "x86";
        metadata[PackageVersionMetadata::UserIntentArchitecture] = "x64";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, x64);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::MachineArchitecture });
    }
    SECTION("x86 installed but x64 as user intent")
    {
        Manifest x86OnlyManifest;
        AddInstaller(x86OnlyManifest, Architecture::X86, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");

        IPackageVersion::Metadata metadata;
        metadata[PackageVersionMetadata::InstalledArchitecture] = "x86";
        metadata[PackageVersionMetadata::UserIntentArchitecture] = "x64";

        ManifestComparator mc(ManifestComparatorTestContext{}, metadata);
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(x86OnlyManifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::MachineArchitecture });
    }
}

TEST_CASE("ManifestComparator_UnsupportedOSArchitecture", "[manifest_comparator]")
{
    auto systemArchitecture = GetSystemArchitecture();
    auto applicableArchitectures = GetApplicableArchitectures();

    // Try to find an applicable architecture that is not the system architecture
    auto itr = std::find_if(applicableArchitectures.begin(), applicableArchitectures.end(), [&](const auto& arch) { return arch != systemArchitecture; });
    if (itr == applicableArchitectures.end())
    {
        // This test requires having an applicable architecture different from the system one.
        // TODO: Does this actually happen in any arch we use?
        return;
    }

    auto applicableArchitecture = *itr;

    Manifest manifest;

    SECTION("System is unsupported")
    {
        ManifestInstaller installer = AddInstaller(manifest, applicableArchitecture, InstallerTypeEnum::Msi, {}, {}, {}, { systemArchitecture });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::MachineArchitecture });
    }
    SECTION("Other is unsupported")
    {
        ManifestInstaller installer = AddInstaller(manifest, applicableArchitecture, InstallerTypeEnum::Msi, {}, {}, {}, { applicableArchitecture });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, installer);
        REQUIRE(inapplicabilities.empty());
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

TEST_CASE("ManifestComparator_MarketFilter", "[manifest_comparator]")
{
    Manifest manifest;

    // Get current market.
    winrt::Windows::Globalization::GeographicRegion region;
    Manifest::string_t currentMarket{ region.CodeTwoLetter() };

    SECTION("Applicable")
    {
        MarketsInfo markets;
        SECTION("Allowed")
        {
            markets.AllowedMarkets = { currentMarket };
        }
        SECTION("Not excluded")
        {
            markets.ExcludedMarkets = { "XX" };
        }

        ManifestInstaller installer = AddInstaller(manifest, Architecture::X86, InstallerTypeEnum::Exe, {}, {}, {}, {}, markets);
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, installer);
        REQUIRE(inapplicabilities.empty());
    }

    SECTION("Inapplicable")
    {
        MarketsInfo markets;
        SECTION("Excluded")
        {
            markets.ExcludedMarkets = { currentMarket };
        }
        SECTION("Not allowed")
        {
            markets.AllowedMarkets = { "XX" };
        }

        ManifestInstaller installer = AddInstaller(manifest, Architecture::X86, InstallerTypeEnum::Exe, {}, {}, {}, {}, markets);
        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::Market});
    }
}

TEST_CASE("ManifestComparator_Scope_AllowUnknown", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller expected = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::Unknown);

    ManifestComparatorTestContext testContext;
    testContext.Args.AddArg(Args::Type::InstallScope, ScopeToString(ScopeEnum::User));

    SECTION("Default")
    {
        ManifestComparator mc(testContext, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::Scope });
    }
    SECTION("Allow Unknown")
    {
        testContext.Add<Data::AllowUnknownScope>(true);

        ManifestComparator mc(testContext, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, expected);
        REQUIRE(inapplicabilities.size() == 0);
    }
}

TEST_CASE("ManifestComparator_InstallerType", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller msi = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User);
    ManifestInstaller exe = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Exe, ScopeEnum::User);
    ManifestInstaller msix = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msix, ScopeEnum::User);

    SECTION("Msi arg requirement")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::InstallerType, "msi"s);

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msi);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstallerType, InapplicabilityFlags::InstallerType });
    }
    SECTION("Msix arg requirement")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::InstallerType, "msix"s);

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msix);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstallerType, InapplicabilityFlags::InstallerType });
    }
    SECTION("Portable arg requirement")
    {
        ManifestComparatorTestContext context;
        context.Args.AddArg(Args::Type::InstallerType, "portable"s);

        ManifestComparator mc(context, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstallerType, InapplicabilityFlags::InstallerType, InapplicabilityFlags::InstallerType });
    }
    SECTION("Exe preference")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallerTypePreference>({ InstallerTypeEnum::Exe });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, exe);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Preference does not exist")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallerTypePreference>({ InstallerTypeEnum::Portable });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msi);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Multiple preferences")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallerTypePreference>({ InstallerTypeEnum::Exe, InstallerTypeEnum::Msix });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, exe);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Multiple preferences alternate order")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallerTypePreference>({ InstallerTypeEnum::Msix, InstallerTypeEnum::Exe });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, msix);
        REQUIRE(inapplicabilities.size() == 0);
    }
    SECTION("Exe requirement")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallerTypeRequirement>({ InstallerTypeEnum::Exe });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        RequireInstaller(result, exe);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstallerType, InapplicabilityFlags::InstallerType });
    }
    SECTION("Inno requirement")
    {
        TestUserSettings settings;
        settings.Set<Setting::InstallerTypeRequirement>({ InstallerTypeEnum::Inno });

        ManifestComparator mc(ManifestComparatorTestContext{}, {});
        auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

        REQUIRE(!result);
        RequireInapplicabilities(inapplicabilities, { InapplicabilityFlags::InstallerType, InapplicabilityFlags::InstallerType, InapplicabilityFlags::InstallerType });
    }
}

TEST_CASE("ManifestComparator_MachineArchitecture_Strong_Scope_Weak", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller system = AddInstaller(manifest, GetSystemArchitecture(), InstallerTypeEnum::Msi, ScopeEnum::Unknown, "", "");
    ManifestInstaller user = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::User, "", "");

    ManifestComparatorTestContext context;

    ManifestComparator mc(context, {});
    auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

    RequireInstaller(result, system);
}

TEST_CASE("ManifestComparator_InstallerCompatibilitySet_Weaker_Than_Architecture", "[manifest_comparator]")
{
    Manifest manifest;
    ManifestInstaller target = AddInstaller(manifest, GetSystemArchitecture(), InstallerTypeEnum::Wix, ScopeEnum::Unknown, "", "");
    ManifestInstaller foil = AddInstaller(manifest, Architecture::Neutral, InstallerTypeEnum::Msi, ScopeEnum::Unknown, "", "");

    ManifestComparatorTestContext context;

    IPackageVersion::Metadata installationMetadata;
    installationMetadata[PackageVersionMetadata::InstalledType] = InstallerTypeToString(foil.EffectiveInstallerType());

    ManifestComparator mc(context, installationMetadata);
    auto [result, inapplicabilities] = mc.GetPreferredInstaller(manifest);

    RequireInstaller(result, target);
}
