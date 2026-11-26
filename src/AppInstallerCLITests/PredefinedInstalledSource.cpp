// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include <ISource.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <Microsoft/PredefinedInstalledSourceFactory.h>
#include <Microsoft/ARPHelper.h>
#include <Microsoft/SQLiteIndexSource.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Runtime;
using namespace AppInstaller::Utility;

using SQLiteIndex = AppInstaller::Repository::Microsoft::SQLiteIndex;
using SQLiteIndexSource = AppInstaller::Repository::Microsoft::SQLiteIndexSource;
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
    REQUIRE(index.GetPropertyByPrimaryId(manifestId, PackageVersionProperty::Name) == entry.DisplayName);
    REQUIRE(index.GetPropertyByPrimaryId(manifestId, PackageVersionProperty::Version) == entry.DisplayVersion);

    REQUIRE(index.GetMultiPropertyByPrimaryId(manifestId, PackageVersionMultiProperty::PackageFamilyName).empty());
    auto productCodes = index.GetMultiPropertyByPrimaryId(manifestId, PackageVersionMultiProperty::ProductCode);
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
    return factory->Create(details)->Open(progress);
}

SQLiteIndex CreateMemoryIndex()
{
    return SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, SQLite::Version::Latest(), SQLiteIndex::CreateOptions::SupportPathless);
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

    auto index = CreateMemoryIndex();
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

    auto index = CreateMemoryIndex();
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

    auto index = CreateMemoryIndex();
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

TEST_CASE("ARPHelper_MSIScopes_Contexts", "[arphelper][msiscope]")
{
    using namespace AppInstaller::Repository::Microsoft;
    auto reset = wil::scope_exit([] { SetEnumMSIProductsOverride({}); });
    const std::wstring userCode = L"{00000000-0000-0000-0000-00000000000a}";
    const std::wstring machineCode = L"{00000000-0000-0000-0000-00000000000B}";
    const std::wstring ambiguousCode = L"{00000000-0000-0000-0000-00000000000C}";
    const std::vector<std::pair<std::wstring, MSIINSTALLCONTEXT>> registrations{
        { userCode, MSIINSTALLCONTEXT_USERMANAGED },
        { L"{00000000-0000-0000-0000-00000000000A}", MSIINSTALLCONTEXT_USERUNMANAGED },
        { machineCode, MSIINSTALLCONTEXT_MACHINE },
        { ambiguousCode, MSIINSTALLCONTEXT_MACHINE },
        { ambiguousCode, MSIINSTALLCONTEXT_USERUNMANAGED },
        { ambiguousCode, MSIINSTALLCONTEXT_MACHINE },
    };
    SetEnumMSIProductsOverride([&](LPCWSTR product, LPCWSTR sid, DWORD contexts, DWORD index, LPWSTR code, MSIINSTALLCONTEXT* context, LPWSTR, LPDWORD) -> UINT
        {
            REQUIRE(product == nullptr);
            REQUIRE(sid == nullptr); // Do not enumerate registrations belonging to other users.
            REQUIRE(contexts == MSIINSTALLCONTEXT_ALL);
            if (index == registrations.size()) { return ERROR_NO_MORE_ITEMS; }
            REQUIRE(index < registrations.size());
            wcscpy_s(code, 39, registrations[index].first.c_str());
            *context = registrations[index].second;
            return ERROR_SUCCESS;
        });

    auto scopes = ARPHelper::GetMSIScopes();
    REQUIRE(scopes.size() == 3);
    REQUIRE(scopes.at(ToLower(ConvertToUTF8(userCode))) == ScopeEnum::User);
    REQUIRE(scopes.at(ToLower(ConvertToUTF8(machineCode))) == ScopeEnum::Machine);
    REQUIRE(scopes.at(ToLower(ConvertToUTF8(ambiguousCode))) == ScopeEnum::Unknown);
}

TEST_CASE("ARPHelper_MSIScopes_EnumerationFailure", "[arphelper][msiscope]")
{
    using namespace AppInstaller::Repository::Microsoft;
    auto reset = wil::scope_exit([] { SetEnumMSIProductsOverride({}); });
    SetEnumMSIProductsOverride([](LPCWSTR, LPCWSTR, DWORD, DWORD index, LPWSTR code, MSIINSTALLCONTEXT* context, LPWSTR, LPDWORD) -> UINT
        {
            if (index == 0)
            {
                wcscpy_s(code, 39, L"{00000000-0000-0000-0000-000000000001}");
                *context = MSIINSTALLCONTEXT_MACHINE;
                return ERROR_SUCCESS;
            }
            return ERROR_ACCESS_DENIED;
        });
    // Do not use a partial snapshot to infer an unambiguous scope.
    REQUIRE(ARPHelper::GetMSIScopes().empty());
}

TEST_CASE("ARPHelper_MSIScopes_SidBufferRetry", "[arphelper][msiscope]")
{
    using namespace AppInstaller::Repository::Microsoft;
    auto reset = wil::scope_exit([] { SetEnumMSIProductsOverride({}); });
    size_t calls = 0;
    SetEnumMSIProductsOverride([&](LPCWSTR, LPCWSTR, DWORD, DWORD index, LPWSTR code, MSIINSTALLCONTEXT* context, LPWSTR, LPDWORD sidSize) -> UINT
        {
            ++calls;
            if (index == 1) { return ERROR_NO_MORE_ITEMS; }
            REQUIRE(index == 0);
            if (calls == 1)
            {
                *sidSize = 300;
                return ERROR_MORE_DATA;
            }
            REQUIRE(*sidSize == 301);
            wcscpy_s(code, 39, L"{00000000-0000-0000-0000-000000000001}");
            *context = MSIINSTALLCONTEXT_USERUNMANAGED;
            return ERROR_SUCCESS;
        });
    REQUIRE(ARPHelper::GetMSIScopes().size() == 1);
    REQUIRE(calls == 3);
}

TEST_CASE("ARPHelper_MSIScopes_InventoryAndFiltering", "[arphelper][msiscope]")
{
    auto root = RegCreateVolatileTestRoot();
    Registry::Key key(root.get());
    ARPHelper helper;
    ARPEntry user("{00000000-0000-0000-0000-00000000000A}", "User MSI", "1.0");
    ARPEntry machine("{00000000-0000-0000-0000-00000000000B}", "Machine MSI", "1.0");
    ARPEntry ambiguous("{00000000-0000-0000-0000-00000000000C}", "Ambiguous MSI", "1.0");
    ARPEntry missing("{00000000-0000-0000-0000-00000000000D}", "Other user or unavailable MSI", "1.0");
    for (auto entry : { user, machine, ambiguous, missing })
    {
        entry.WindowsInstaller = true;
        AddARPEntryToKey(root.get(), helper, entry);
    }
    AddARPEntryToKey(root.get(), helper, ARPEntry{ "NonMsi", "Machine EXE", "1.0" });
    ARPHelper::MSIScopeMap scopes{
        { ToLower(user.EntryName), ScopeEnum::User },
        { ToLower(machine.EntryName), ScopeEnum::Machine },
        { ToLower(ambiguous.EntryName), ScopeEnum::Unknown },
    };

    auto filter = ScopeEnum::Unknown;
    size_t expected = 3;
    SECTION("Unfiltered") {}
    SECTION("User filter includes user MSI from machine ARP") { filter = ScopeEnum::User; expected = 1; }
    SECTION("Machine filter excludes user MSI") { filter = ScopeEnum::Machine; expected = 2; }
    SECTION("Failed enumeration excludes MSIs but retains EXEs") { scopes.clear(); expected = 1; }

    auto index = CreateMemoryIndex();
    helper.PopulateIndexFromKey(index, key, ScopeToString(ScopeEnum::Machine), "X64", {}, &scopes, filter);
    auto results = index.Search({});
    REQUIRE(results.Matches.size() == expected);
    for (const auto& match : results.Matches)
    {
        auto name = index.GetPropertyByPrimaryId(match.first, PackageVersionProperty::Name);
        auto metadata = index.GetMetadataByManifestId(match.first);
        auto scope = Find(metadata, PackageVersionMetadata::InstalledScope);
        REQUIRE(scope != metadata.end());
        REQUIRE(scope->second == ScopeToString(name == user.DisplayName ? ScopeEnum::User : ScopeEnum::Machine));
        REQUIRE(name != ambiguous.DisplayName);
        REQUIRE(name != missing.DisplayName);
    }
}

TEST_CASE("PredefinedInstalledSource_UserFilterScansMachineARPForUserMSI", "[installed][msiscope]")
{
    using namespace AppInstaller::Repository::Microsoft;
    auto root = RegCreateVolatileTestRoot();
    ARPHelper helper;
    ARPEntry entry("{00000000-0000-0000-0000-000000000001}", "User MSI in machine ARP", "1.0");
    entry.WindowsInstaller = true;
    AddARPEntryToKey(root.get(), helper, entry);
    auto reset = wil::scope_exit([] { SetGetARPKeyOverride({}); SetEnumMSIProductsOverride({}); });
    SetGetARPKeyOverride([&](ScopeEnum scope, Architecture arch)
        {
            return scope == ScopeEnum::Machine && arch == GetSystemArchitecture() ? Registry::Key{ root.get() } : Registry::Key{};
        });
    SetEnumMSIProductsOverride([](LPCWSTR, LPCWSTR, DWORD, DWORD index, LPWSTR code, MSIINSTALLCONTEXT* context, LPWSTR, LPDWORD) -> UINT
        {
            if (index != 0) { return ERROR_NO_MORE_ITEMS; }
            wcscpy_s(code, 39, L"{00000000-0000-0000-0000-000000000001}");
            *context = MSIINSTALLCONTEXT_USERUNMANAGED;
            return ERROR_SUCCESS;
        });
    SearchRequest request;
    request.Inclusions.emplace_back(PackageMatchField::ProductCode, MatchType::Exact, entry.EntryName);
    SECTION("User filter")
    {
        auto source = CreatePredefinedInstalledSource(Factory::Filter::User);
        REQUIRE(source->Search(request).Matches.size() == 1);
    }
    SECTION("Machine filter")
    {
        auto source = CreatePredefinedInstalledSource(Factory::Filter::Machine);
        REQUIRE(source->Search(request).Matches.empty());
    }
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

std::string GetDatabaseIdentifier(const std::shared_ptr<Repository::ISource>& source)
{
    return reinterpret_cast<SQLiteIndexSource*>(source->CastTo(ISourceType::SQLiteIndexSource))->GetIndex().GetDatabaseIdentifier();
}

void RequirePackagesHaveSameNames(std::shared_ptr<ISource>& source1, std::shared_ptr<ISource>& source2)
{
    auto result1 = source1->Search({});
    REQUIRE(!result1.Matches.empty());

    // Ensure that all packages have the same name values
    for (const auto& match : result1.Matches)
    {
        std::string packageId = match.Package->GetProperty(PackageProperty::Id).get();
        INFO(packageId);

        SearchRequest id2;
        id2.Inclusions.emplace_back(PackageMatchFilter{ PackageMatchField::Id, MatchType::CaseInsensitive, packageId });
        auto result2 = source2->Search(id2);
        REQUIRE(result2.Matches.size() == 1);
        REQUIRE(match.Package->GetProperty(PackageProperty::Name) == result2.Matches[0].Package->GetProperty(PackageProperty::Name));
    }
}

TEST_CASE("PredefinedInstalledSource_Create_Cached", "[installed][list][installed-cache]")
{
    auto source1 = CreatePredefinedInstalledSource();
    auto source2 = CreatePredefinedInstalledSource();

    // Ensure the same identifier (which should mean the cache was not updated)
    REQUIRE(
        GetDatabaseIdentifier(source1)
        ==
        GetDatabaseIdentifier(source2)
    );

    RequirePackagesHaveSameNames(source1, source2);
    RequirePackagesHaveSameNames(source2, source1);
}

TEST_CASE("PredefinedInstalledSource_Create_ForceCacheUpdate", "[installed][list][installed-cache]")
{
    auto source1 = CreatePredefinedInstalledSource();
    auto source2 = CreatePredefinedInstalledSource(Factory::Filter::NoneWithForcedCacheUpdate);

    // Ensure different identifier (which should mean the cache was updated)
    REQUIRE(
        GetDatabaseIdentifier(source1)
        !=
        GetDatabaseIdentifier(source2)
    );

    RequirePackagesHaveSameNames(source1, source2);
    RequirePackagesHaveSameNames(source2, source1);
}

TEST_CASE("PredefinedInstalledSource_Create_ForceCacheUpdate_StillCached", "[installed][list][installed-cache]")
{
    auto source1 = CreatePredefinedInstalledSource();
    auto source2 = CreatePredefinedInstalledSource(Factory::Filter::NoneWithForcedCacheUpdate);
    auto source3 = CreatePredefinedInstalledSource();

    CAPTURE(GetDatabaseIdentifier(source1), GetDatabaseIdentifier(source2), GetDatabaseIdentifier(source3));

    // Ensure different identifier (which should mean the cache was updated)
    REQUIRE(
        GetDatabaseIdentifier(source1)
        !=
        GetDatabaseIdentifier(source2)
    );

    // Ensure the same identifier (which should mean the cache was not updated)
    REQUIRE(
        GetDatabaseIdentifier(source2)
        ==
        GetDatabaseIdentifier(source3)
    );
}
