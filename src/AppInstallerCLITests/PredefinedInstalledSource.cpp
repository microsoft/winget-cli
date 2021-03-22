// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerRepositorySource.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <Microsoft/PredefinedInstalledSourceFactory.h>
#include <Microsoft/ARPHelper.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Runtime;
using namespace AppInstaller::Utility;

using SQLiteIndex = AppInstaller::Repository::Microsoft::SQLiteIndex;
using Factory = AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory;
using ARPHelper = AppInstaller::Repository::Microsoft::ARPHelper;

constexpr std::string_view s_TestScope = "TestScope"sv;

struct ARPEntry
{
    ARPEntry(std::string entryName) : EntryName(std::move(entryName)) {}
    ARPEntry(std::string entryName, std::optional<std::string> displayName, std::optional<std::string> displayVersion, bool systemComponent = false) :
        EntryName(std::move(entryName)), DisplayName(std::move(displayName)), DisplayVersion(std::move(displayVersion)), SystemComponent(systemComponent) {}

    std::string EntryName;
    std::optional<std::string> DisplayName;
    std::optional<std::string> DisplayVersion;
    std::optional<std::string> Publisher;
    std::optional<std::string> InstallLocation;
    std::optional<std::string> UninstallString;
    std::optional<std::string> QuietUninstallString;
    std::optional<bool> WindowsInstaller;
    std::optional<bool> SystemComponent;
};

void AddARPValueToKey(HKEY key, const std::wstring& name, const std::optional<std::string>& value)
{
    if (value)
    {
        SetRegistryValue(key, name, ConvertToUTF16(value.value()));
    }
}

void AddARPValueToKey(HKEY key, const std::wstring& name, const std::optional<bool>& value)
{
    if (value)
    {
        SetRegistryValue(key, name, (value.value() ? 1 : 0));
    }
}

void AddARPEntryToKey(HKEY key, const ARPHelper& helper, const ARPEntry& entry)
{
    auto subkey = RegCreateVolatileSubKey(key, ConvertToUTF16(entry.EntryName));

#define ADD_ARP_VALUE(_name_) AddARPValueToKey(subkey.get(), helper._name_, entry._name_)
    ADD_ARP_VALUE(DisplayName);
    ADD_ARP_VALUE(DisplayVersion);
    ADD_ARP_VALUE(Publisher);
    ADD_ARP_VALUE(InstallLocation);
    ADD_ARP_VALUE(UninstallString);
    ADD_ARP_VALUE(QuietUninstallString);
    ADD_ARP_VALUE(WindowsInstaller);
    ADD_ARP_VALUE(SystemComponent);
#undef ADD_ARP_VALUE
}

void AddARPEntriesToKey(HKEY key, const ARPHelper& helper, const std::vector<ARPEntry>& entries)
{
    for (const auto& entry : entries)
    {
        AddARPEntryToKey(key, helper, entry);
    }
}

SQLiteIndex::MetadataResult::const_iterator Find(const SQLiteIndex::MetadataResult& metadata, PackageVersionMetadata value)
{
    return std::find_if(metadata.begin(), metadata.end(), [value](const auto& m) { return m.first == value; });
}

void VerifyInstalledType(const SQLiteIndex::MetadataResult& metadata, InstallerTypeEnum type)
{
    auto itr = Find(metadata, PackageVersionMetadata::InstalledType);
    REQUIRE(itr != metadata.end());
    REQUIRE(ConvertToInstallerTypeEnum(itr->second) == type);
}

void VerifyTestScope(const SQLiteIndex::MetadataResult& metadata)
{
    auto itr = Find(metadata, PackageVersionMetadata::InstalledScope);
    REQUIRE(itr != metadata.end());
    REQUIRE(itr->second == s_TestScope);
}

void VerifyMetadataString(const SQLiteIndex::MetadataResult& metadata, PackageVersionMetadata pvm, const std::optional<std::string>& value)
{
    auto itr = Find(metadata, pvm);
    if (value)
    {
        REQUIRE(itr != metadata.end());
        REQUIRE(itr->second == value.value());
    }
    else
    {
        REQUIRE(itr == metadata.end());
    }
}

void VerifyEntryAgainstIndex(const SQLiteIndex& index, SQLiteIndex::IdType manifestId, const ARPEntry& entry)
{
    REQUIRE(index.GetPropertyByManifestId(manifestId, PackageVersionProperty::Name) == entry.DisplayName);
    REQUIRE(index.GetPropertyByManifestId(manifestId, PackageVersionProperty::Version) == entry.DisplayVersion);

    REQUIRE(index.GetMultiPropertyByManifestId(manifestId, PackageVersionMultiProperty::PackageFamilyName).empty());
    auto productCodes = index.GetMultiPropertyByManifestId(manifestId, PackageVersionMultiProperty::ProductCode);
    REQUIRE(productCodes.size() == 1);
    REQUIRE(productCodes[0] == FoldCase(static_cast<std::string_view>(entry.EntryName)));

    auto metadata = index.GetMetadataByManifestId(manifestId);

    VerifyInstalledType(metadata, entry.WindowsInstaller.value_or(false) ? InstallerTypeEnum::Msi : InstallerTypeEnum::Exe);
    VerifyTestScope(metadata);
    VerifyMetadataString(metadata, PackageVersionMetadata::Publisher, entry.Publisher);
    VerifyMetadataString(metadata, PackageVersionMetadata::InstalledLocation, entry.InstallLocation);
    VerifyMetadataString(metadata, PackageVersionMetadata::StandardUninstallCommand, entry.UninstallString);
    VerifyMetadataString(metadata, PackageVersionMetadata::SilentUninstallCommand, entry.QuietUninstallString);
}

std::shared_ptr<ISource> CreatePredefinedInstalledSource(Factory::Filter filter = Factory::Filter::None)
{
    SourceDetails details;
    details.Type = Factory::Type();
    details.Arg = Factory::FilterToString(filter);

    TestProgress progress;

    auto factory = Factory::Create();
    return factory->Create(details, progress);
}

TEST_CASE("ARPHelper_GetARPForArchitecture", "[arphelper][list]")
{
    auto systemArch = GetSystemArchitecture();

    ARPHelper helper;

    auto nativeMachineKey = helper.GetARPKey(ScopeEnum::Machine, systemArch);
    REQUIRE(nativeMachineKey);
}

TEST_CASE("ARPHelper_GetBoolValue_DoesNotExist", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());
    std::wstring valueName = L"TestValueName";

    ARPHelper helper;

    REQUIRE_FALSE(helper.GetBoolValue(key, valueName));
}

TEST_CASE("ARPHelper_GetBoolValue_NotDword", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());
    std::wstring valueName = L"TestValueName";

    SetRegistryValue(root.get(), valueName, L"True");

    ARPHelper helper;

    REQUIRE_FALSE(helper.GetBoolValue(key, valueName));
}

TEST_CASE("ARPHelper_GetBoolValue_Zero", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());
    std::wstring valueName = L"TestValueName";

    SetRegistryValue(root.get(), valueName, 0);

    ARPHelper helper;

    REQUIRE_FALSE(helper.GetBoolValue(key, valueName));
}

TEST_CASE("ARPHelper_GetBoolValue_One", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());
    std::wstring valueName = L"TestValueName";

    SetRegistryValue(root.get(), valueName, 1);

    ARPHelper helper;

    REQUIRE(helper.GetBoolValue(key, valueName));
}

TEST_CASE("ARPHelper_GetBoolValue_FortyTwo", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());
    std::wstring valueName = L"TestValueName";

    SetRegistryValue(root.get(), valueName, 42);

    ARPHelper helper;

    REQUIRE(helper.GetBoolValue(key, valueName));
}

TEST_CASE("ARPHelper_DetermineVersion_DisplayVersion", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());

    ARPHelper helper;

    SetRegistryValue(root.get(), helper.DisplayVersion, L"1.0");
    SetRegistryValue(root.get(), helper.Version, 0x0207002A);
    SetRegistryValue(root.get(), helper.VersionMajor, 3);
    SetRegistryValue(root.get(), helper.VersionMinor, 14);

    auto result = helper.DetermineVersion(key);
    REQUIRE(result == "1.0");
}

TEST_CASE("ARPHelper_DetermineVersion_Version", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());

    ARPHelper helper;

    SetRegistryValue(root.get(), helper.Version, 0x0207002A);
    SetRegistryValue(root.get(), helper.VersionMajor, 3);
    SetRegistryValue(root.get(), helper.VersionMinor, 14);

    auto result = helper.DetermineVersion(key);
    REQUIRE(result == "3.14");
}

TEST_CASE("ARPHelper_DetermineVersion_VersionMajorMinor", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());

    ARPHelper helper;

    SetRegistryValue(root.get(), helper.VersionMajor, 3);
    SetRegistryValue(root.get(), helper.VersionMinor, 14);

    auto result = helper.DetermineVersion(key);
    REQUIRE(result == "3.14");
}

TEST_CASE("ARPHelper_DetermineVersion_Unknown", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());

    ARPHelper helper;

    auto result = helper.DetermineVersion(key);
    REQUIRE(result == Version::CreateUnknown().ToString());
}

TEST_CASE("ARPHelper_PopulateIndexFromKey_Single", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());

    ARPHelper helper;

    // Create a single ARP entry under the root
    ARPEntry entry("SingleEntry");

    entry.DisplayName = "Test Name";
    entry.DisplayVersion = "1.2";
    entry.Publisher = "Test Publisher";
    entry.InstallLocation = "TestLocation";
    entry.UninstallString = "Test Uninstall";
    entry.QuietUninstallString = "Test Quiet Uninstall";
    entry.WindowsInstaller = true;

    AddARPEntryToKey(root.get(), helper, entry);

    auto index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET);
    helper.PopulateIndexFromKey(index, key, s_TestScope, "TestArchitecture");

    auto result = index.Search({});

    REQUIRE(result.Matches.size() == 1);
    VerifyEntryAgainstIndex(index, result.Matches[0].first, entry);
}

TEST_CASE("ARPHelper_PopulateIndexFromKey_SingleValid", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());

    ARPHelper helper;

    // Create a single ARP entry under the root
    ARPEntry entry("SingleEntry");

    entry.DisplayName = "Test Name";
    entry.DisplayVersion = "1.2";
    entry.Publisher = "Test Publisher";
    entry.InstallLocation = "TestLocation";
    entry.UninstallString = "Test Uninstall";
    entry.QuietUninstallString = "Test Quiet Uninstall";
    entry.WindowsInstaller = false;

    AddARPEntryToKey(root.get(), helper, entry);

    // Name and version must exist, as well as not being a system component.
    AddARPEntriesToKey(root.get(), helper, {
        { "ValidButIsSystemComponent", "A", "0.1", true },
        { "NoName", {}, "0.2" },
        { "Nothing" },
        });

    auto index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET);
    helper.PopulateIndexFromKey(index, key, s_TestScope, "TestArchitecture");

    auto result = index.Search({});

    REQUIRE(result.Matches.size() == 1);
    VerifyEntryAgainstIndex(index, result.Matches[0].first, entry);
}

TEST_CASE("ARPHelper_PopulateIndexFromKey_Two", "[arphelper][list]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());

    ARPHelper helper;

    ARPEntry entry1("FirstEntry");
    entry1.DisplayName = "Test Name";
    entry1.DisplayVersion = "1.2";
    entry1.Publisher = "Test Publisher";
    entry1.InstallLocation = "TestLocation";
    entry1.UninstallString = "Test Uninstall";
    entry1.QuietUninstallString = "Test Quiet Uninstall";
    entry1.WindowsInstaller = true;

    ARPEntry entry2("SecondEntry");
    entry2.DisplayName = "Different Test Name";
    entry2.DisplayVersion = "31.4";
    entry2.Publisher = "Different Test Publisher";
    entry2.InstallLocation = "DifferentTestLocation";
    entry2.UninstallString = "Different Test Uninstall";
    entry2.QuietUninstallString = "Different Test Quiet Uninstall";

    AddARPEntryToKey(root.get(), helper, entry1);
    AddARPEntryToKey(root.get(), helper, entry2);

    auto index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET);
    helper.PopulateIndexFromKey(index, key, s_TestScope, "TestArchitecture");

    REQUIRE(index.Search({}).Matches.size() == 2);

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, entry1.EntryName);
    auto result = index.Search(request);

    REQUIRE(result.Matches.size() == 1);
    VerifyEntryAgainstIndex(index, result.Matches[0].first, entry1);

    request.Query = RequestMatch(MatchType::Exact, entry2.EntryName);
    result = index.Search(request);

    REQUIRE(result.Matches.size() == 1);
    VerifyEntryAgainstIndex(index, result.Matches[0].first, entry2);
}

TEST_CASE("PredefinedInstalledSource_Create", "[installed][list]")
{
    auto source = CreatePredefinedInstalledSource();
}

TEST_CASE("PredefinedInstalledSource_Search", "[installed][list]")
{
    auto source = CreatePredefinedInstalledSource();

    SearchRequest request;

    auto results = source->Search(request);

    REQUIRE_FALSE(results.Matches.empty());
}
