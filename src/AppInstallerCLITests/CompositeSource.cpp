// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include "TestHooks.h"
#include <CompositeSource.h>
#include <Microsoft/SQLiteIndexSource.h>
#include <Microsoft/PinningIndex.h>
#include <PackageTrackingCatalogSourceFactory.h>
#include <winget/Pin.h>
#include <winget/PinningData.h>
#include <winget/PackageVersionSelection.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Pinning;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Utility;

constexpr std::string_view s_Everything_Query = "everything"sv;

// A test source that has two modes:
//  1. A request that IsForEverything returns the stored result. This models the
//      incoming search request to a CompositeSource.
//  2. A request that is not for everything invokes TestSource::SearchFunction to
//      enable verification of expectations.
struct ComponentTestSource : public TestSource
{
    ComponentTestSource() = default;

    ComponentTestSource(std::string_view identifier, SourceOrigin origin = SourceOrigin::Default)
    {
        Details.Identifier = identifier;
        Details.Origin = origin;
    }

    SearchResult Search(const SearchRequest& request) const override
    {
        if (request.Query && request.Query.value().Value == s_Everything_Query)
        {
            return Everything;
        }
        else
        {
            return TestSource::Search(request);
        }
    }

    SearchResult Everything;
};

// A helper to make matches.
struct Criteria : public PackageMatchFilter
{
    Criteria() : PackageMatchFilter(PackageMatchField::Id, MatchType::Wildcard, ""sv) {}
    Criteria(PackageMatchField field) : PackageMatchFilter(field, MatchType::Wildcard, ""sv) {}
};

Manifest::Manifest MakeDefaultManifest(std::string_view version = "1.0"sv)
{
    Manifest::Manifest result;

    result.Id = "Id";
    result.DefaultLocalization.Add<Manifest::Localization::PackageName>("Name");
    result.DefaultLocalization.Add<Manifest::Localization::Publisher>("Publisher");
    result.Version = version;
    result.Installers.push_back({});

    return result;
}

struct TestPackageHelper
{
    TestPackageHelper(bool isInstalled, std::shared_ptr<ISource> source = {}) :
        m_isInstalled(isInstalled), m_manifest(MakeDefaultManifest()), m_source(source) {}

    TestPackageHelper& WithId(const std::string& id)
    {
        m_manifest.Id = id;
        return *this;
    }

    TestPackageHelper& WithVersion(std::string_view version)
    {
        m_manifest.Version = version;
        return *this;
    }

    TestPackageHelper& WithChannel(const std::string& channel)
    {
        m_manifest.Channel = channel;
        return *this;
    }

    TestPackageHelper& WithDefaultName(std::string_view name)
    {
        m_manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(std::string{ name });
        return *this;
    }

    TestPackageHelper& WithPFN(const std::string& pfn)
    {
        m_manifest.Installers[0].PackageFamilyName = pfn;
        return *this;
    }

    TestPackageHelper& WithPC(const std::string& pc)
    {
        m_manifest.Installers[0].ProductCode = pc;
        return *this;
    }

    TestPackageHelper& HideSRS(bool value = true)
    {
        m_hideSystemReferenceStrings = value;
        return *this;
    }

    std::shared_ptr<TestCompositePackage> ToPackage()
    {
        if (!m_package)
        {
            if (m_isInstalled)
            {
                m_package = TestCompositePackage::Make(m_manifest, TestCompositePackage::MetadataMap{}, std::vector<Manifest::Manifest>(), m_source);
            }
            else
            {
                m_package = TestCompositePackage::Make(std::vector<Manifest::Manifest>{ m_manifest }, m_source, m_hideSystemReferenceStrings);
            }
        }

        return m_package;
    }

    operator std::shared_ptr<ICompositePackage>()
    {
        return ToPackage();
    }

    operator const Manifest::Manifest& () const
    {
        return m_manifest;
    }

private:
    bool m_isInstalled;
    Manifest::Manifest m_manifest;
    std::shared_ptr<ISource> m_source;
    std::shared_ptr<TestCompositePackage> m_package;
    bool m_hideSystemReferenceStrings = false;
};

// A helper to create the sources used by the majority of tests in this file.
struct CompositeTestSetup
{
    CompositeTestSetup(CompositeSearchBehavior behavior = CompositeSearchBehavior::Installed) : Composite("*Tests")
    {
        Installed = std::make_shared<ComponentTestSource>("InstalledTestSource1", SourceOrigin::Predefined);
        Available = std::make_shared<ComponentTestSource>("AvailableTestSource1");
        Composite.SetInstalledSource(Source{ Installed }, behavior);
        Composite.AddAvailableSource(Source{ Available });
    }

    SearchResult Search()
    {
        SearchRequest request;
        request.Query = RequestMatch(MatchType::Exact, s_Everything_Query);
        return Composite.Search(request);
    }

    TestPackageHelper MakeInstalled(std::shared_ptr<ISource> source)
    {
        return { /* isInstalled */ true, std::move(source)};
    }

    TestPackageHelper MakeInstalled()
    {
        return MakeInstalled(Installed);
    }

    TestPackageHelper MakeAvailable(std::shared_ptr<ISource> source)
    {
        return { /* isInstalled */ false, std::move(source) };
    }

    TestPackageHelper MakeAvailable()
    {
        return MakeAvailable(Available);
    }

    std::shared_ptr<ComponentTestSource> Installed;
    std::shared_ptr<ComponentTestSource> Available;
    CompositeSource Composite;
};

// A helper to create the sources used by the majority of tests in this file.
struct CompositeWithTrackingTestSetup : public CompositeTestSetup
{
    CompositeWithTrackingTestSetup() : TrackingFactory([&](const SourceDetails&) { return Tracking; })
    {
        Tracking = std::make_shared<SQLiteIndexSource>(SourceDetails{}, SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET));
        TestHook_SetSourceFactoryOverride(std::string{ PackageTrackingCatalogSourceFactory::Type() }, TrackingFactory);
    }

    ~CompositeWithTrackingTestSetup()
    {
        TestHook_ClearSourceFactoryOverrides();
    }

    TestSourceFactory TrackingFactory;
    std::shared_ptr<SQLiteIndexSource> Tracking;
};

bool SearchRequestIncludes(const std::vector<PackageMatchFilter>& filters, PackageMatchField field, MatchType type, std::optional<std::string> value = {})
{
    bool found = false;

    for (const PackageMatchFilter& filter : filters)
    {
        if (filter.Field == field && filter.Type == type &&
            (!value || filter.Value == value.value()))
        {
            found = true;
        }
    }

    return found;
}

void RequireSearchRequestIncludes(const std::vector<PackageMatchFilter>& filters, PackageMatchField field, MatchType type, std::optional<std::string> value = {})
{
    REQUIRE(SearchRequestIncludes(filters, field, type, value));
}

TEST_CASE("CompositeSource_PackageFamilyName_NotAvailable", "[CompositeSource]")
{
    // Pre-folded for easier ==
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().empty());
}

TEST_CASE("CompositeSource_PackageFamilyName_Available", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithPFN(pfn), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_ProductCode_NotAvailable", "[CompositeSource]")
{
    std::string pc = "thiscouldbeapc";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPC(pc), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().empty());
}

TEST_CASE("CompositeSource_ProductCode_Available", "[CompositeSource]")
{
    std::string pc = "thiscouldbeapc";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPC(pc), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::ProductCode, MatchType::Exact, pc);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithPC(pc), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_NameAndPublisher_Match", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled(), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable(), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_MultiMatch_FindsStrongMatch", "[CompositeSource]")
{
    std::string name = "MatchingName";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN("sortof_apfn"), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithId("A different ID"), Criteria(PackageMatchField::NormalizedNameAndPublisher));
        result.Matches.emplace_back(setup.MakeAvailable().WithDefaultName(name), Criteria(PackageMatchField::PackageFamilyName));
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
    auto version = result.Matches[0].Package->GetAvailable()[0]->GetLatestVersion();
    REQUIRE(version->GetProperty(PackageVersionProperty::Name).get() == name);
    REQUIRE(!Version(version->GetProperty(PackageVersionProperty::Version)).IsUnknown());
}

TEST_CASE("CompositeSource_MultiMatch_DoesNotFindStrongMatch", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN("sortof_apfn"), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithId("A different ID"), Criteria(PackageMatchField::NormalizedNameAndPublisher));
        result.Matches.emplace_back(setup.MakeAvailable().WithId("Another diff ID"), Criteria(PackageMatchField::NormalizedNameAndPublisher));
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().empty());
}

TEST_CASE("CompositeSource_FoundByBothRootSearches", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);
    auto availablePackage = setup.MakeAvailable().WithPFN(pfn);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_OnlyAvailableFoundByRootSearch", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria(PackageMatchField::PackageFamilyName));
        return result;
    };

    std::shared_ptr<TestCompositePackage> availablePackage = setup.MakeAvailable().WithPFN(pfn).ToPackage();
    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria(PackageMatchField::PackageFamilyName));
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_FoundByAvailableRootSearch_NotInstalled", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Available->Everything.Matches.emplace_back(setup.MakeAvailable().WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithPFN(pfn), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.empty());
}

TEST_CASE("CompositeSource_UpdateWithBetterMatchCriteria", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    MatchType originalType = MatchType::Wildcard;
    MatchType type = MatchType::Exact;

    CompositeTestSetup setup;
    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);
    auto availablePackage = setup.MakeAvailable().WithPFN(pfn);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].MatchCriteria.Type == originalType);

    // Now make the source root search find it with a better criteria
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, PackageMatchFilter(PackageMatchField::Id, type, ""sv));

    result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].MatchCriteria.Type == type);
}

TEST_CASE("CompositePackage_PropertyFromInstalled", "[CompositeSource]")
{
    std::string id = "Special test ID";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithId(id), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetProperty(PackageProperty::Id) == id);
}

TEST_CASE("CompositePackage_PropertyFromAvailable", "[CompositeSource]")
{
    std::string id = "Special test ID";
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithId(id), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetProperty(PackageProperty::Id) == id);
}

TEST_CASE("CompositePackage_AvailableVersions_ChannelFilteredOut", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string channel = "Channel";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        Manifest::Manifest noChannel = MakeDefaultManifest();
        noChannel.Version = "1.0";

        Manifest::Manifest hasChannel = MakeDefaultManifest();
        hasChannel.Channel = channel;
        hasChannel.Version = "2.0";

        SearchResult result;
        result.Matches.emplace_back(TestCompositePackage::Make(std::vector<Manifest::Manifest>{ noChannel, hasChannel }, setup.Available), Criteria());
        REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
        REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 2);
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    auto package = result.Matches[0].Package->GetAvailable()[0];

    auto versionKeys = package->GetVersionKeys();
    REQUIRE(versionKeys.size() == 2);

    auto availableVersions = GetAvailableVersionsForInstalledVersion(result.Matches[0].Package);
    auto availableVersionKeys = availableVersions->GetVersionKeys();
    REQUIRE(availableVersionKeys.size() == 1);
    REQUIRE(availableVersionKeys[0].Channel.empty());

    auto latestVersion = availableVersions->GetLatestVersion();
    REQUIRE(latestVersion);
    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Channel).get().empty());
}

TEST_CASE("CompositePackage_AvailableVersions_NoChannelFilteredOut", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string channel = "Channel";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn).WithChannel(channel), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        Manifest::Manifest noChannel = MakeDefaultManifest();
        noChannel.Version = "1.0";

        Manifest::Manifest hasChannel = MakeDefaultManifest();
        hasChannel.Channel = channel;
        hasChannel.Version = "2.0";

        SearchResult result;
        result.Matches.emplace_back(TestCompositePackage::Make(std::vector<Manifest::Manifest>{ noChannel, hasChannel }, setup.Available), Criteria());
        REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
        REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 2);
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    auto package = result.Matches[0].Package->GetAvailable()[0];

    auto versionKeys = package->GetVersionKeys();
    REQUIRE(versionKeys.size() == 2);

    auto availableVersions = GetAvailableVersionsForInstalledVersion(result.Matches[0].Package);
    auto availableVersionKeys = availableVersions->GetVersionKeys();
    REQUIRE(availableVersionKeys.size() == 1);
    REQUIRE(availableVersionKeys[0].Channel == channel);

    auto latestVersion = availableVersions->GetLatestVersion();
    REQUIRE(latestVersion);
    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Channel).get() == channel);
}

TEST_CASE("CompositeSource_MultipleAvailableSources_MatchAll", "[CompositeSource]")
{
    TestCommon::TestUserSettings testSettings;

    std::string pfn = "sortof_apfn";
    std::string firstName = "Name1";
    std::string secondName = "Name2";

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(Source{ secondAvailable });

    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithDefaultName(firstName), Criteria());
        return result;
    };

    secondAvailable->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable(secondAvailable).WithDefaultName(secondName), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 2);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetProperty(PackageProperty::Name).get() == firstName);
    REQUIRE(result.Matches[0].Package->GetAvailable()[1]->GetProperty(PackageProperty::Name).get() == secondName);
}

TEST_CASE("CompositeSource_MultipleAvailableSources_MatchSecond", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string firstName = "Name1";
    std::string secondName = "Name2";

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(Source{ secondAvailable });

    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria());

    secondAvailable->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithDefaultName(secondName), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetProperty(PackageProperty::Name).get() == secondName);
}

TEST_CASE("CompositeSource_MultipleAvailableSources_ReverseMatchBoth", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);

    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(Source{ secondAvailable });

    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria(PackageMatchField::PackageFamilyName));
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(setup.MakeAvailable().WithPFN(pfn), Criteria());
    secondAvailable->Everything.Matches.emplace_back(setup.MakeAvailable().WithPFN(pfn), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 2);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[1]->GetVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_IsSame", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN("sortof_apfn"), Criteria());

    SearchResult result1 = setup.Search();
    REQUIRE(result1.Matches.size() == 1);

    SearchResult result2 = setup.Search();
    REQUIRE(result2.Matches.size() == 1);

    REQUIRE(result1.Matches[0].Package->GetInstalled());
    REQUIRE(result1.Matches[0].Package->GetInstalled()->IsSame(result1.Matches[0].Package->GetInstalled().get()));
}

TEST_CASE("CompositeSource_AvailableSearchFailure", "[CompositeSource]")
{
    HRESULT expectedHR = E_BLUETOOTH_ATT_ATTRIBUTE_NOT_FOUND;
    std::string pfn = "sortof_apfn";

    std::shared_ptr<ComponentTestSource> AvailableSucceeds = std::make_shared<ComponentTestSource>();
    AvailableSucceeds->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(TestPackageHelper{ /* isInstalled */ false }.WithPFN(pfn), Criteria());
        return result;
    };

    std::shared_ptr<ComponentTestSource> AvailableFails = std::make_shared<ComponentTestSource>();
    AvailableFails->SearchFunction = [&](const SearchRequest&) -> SearchResult { THROW_HR(expectedHR); };
    AvailableFails->Details.Name = "The one that fails";

    CompositeSource Composite("*CompositeSource_AvailableSearchFailure");
    Composite.AddAvailableSource(Source{ AvailableSucceeds });
    Composite.AddAvailableSource(Source{ AvailableFails });

    SearchResult result = Composite.Search({});

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);

    auto pfns = result.Matches[0].Package->GetAvailable()[0]->GetLatestVersion()->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
    REQUIRE(pfns.size() == 1);
    REQUIRE(pfns[0] == pfn);

    REQUIRE(result.Failures.size() == 1);
    REQUIRE(result.Failures[0].SourceName == AvailableFails->Details.Name);

    HRESULT searchFailure = S_OK;
    try
    {
        std::rethrow_exception(result.Failures[0].Exception);
    }
    catch (const wil::ResultException& re)
    {
        searchFailure = re.GetErrorCode();
    }
    catch (...) {}

    REQUIRE(searchFailure == expectedHR);
}

TEST_CASE("CompositeSource_InstalledToAvailableCorrelationSearchFailure", "[CompositeSource]")
{
    HRESULT expectedHR = E_BLUETOOTH_ATT_ATTRIBUTE_NOT_LONG;
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->Everything.Matches.emplace_back(setup.MakeAvailable().WithPFN(pfn), Criteria());

    std::shared_ptr<ComponentTestSource> AvailableFails = std::make_shared<ComponentTestSource>();
    AvailableFails->SearchFunction = [&](const SearchRequest&) -> SearchResult { THROW_HR(expectedHR); };
    AvailableFails->Details.Name = "The one that fails";

    setup.Composite.AddAvailableSource(Source{ AvailableFails });

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);

    REQUIRE(result.Failures.size() == 1);
    REQUIRE(result.Failures[0].SourceName == AvailableFails->Details.Name);

    HRESULT searchFailure = S_OK;
    try
    {
        std::rethrow_exception(result.Failures[0].Exception);
    }
    catch (const wil::ResultException& re)
    {
        searchFailure = re.GetErrorCode();
    }
    catch (...) {}

    REQUIRE(searchFailure == expectedHR);
}

TEST_CASE("CompositeSource_InstalledAvailableSearchFailure", "[CompositeSource]")
{
    HRESULT expectedHR = E_BLUETOOTH_ATT_ATTRIBUTE_NOT_LONG;
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithPFN(pfn), Criteria());
        return result;
    };

    std::shared_ptr<ComponentTestSource> AvailableFails = std::make_shared<ComponentTestSource>();
    AvailableFails->SearchFunction = [&](const SearchRequest&) -> SearchResult { THROW_HR(expectedHR); };
    AvailableFails->Details.Name = "The one that fails";

    setup.Composite.AddAvailableSource(Source{ AvailableFails });

    setup.Composite.SetInstalledSource(Source{ setup.Installed }, CompositeSearchBehavior::AvailablePackages);

    SearchRequest request;
    request.Query = RequestMatch{ MatchType::Exact, "whatever" };
    SearchResult result = setup.Composite.Search(request);

    REQUIRE(result.Matches.size() == 1);

    REQUIRE(result.Failures.size() == 1);
    REQUIRE(result.Failures[0].SourceName == AvailableFails->Details.Name);

    HRESULT searchFailure = S_OK;
    try
    {
        std::rethrow_exception(result.Failures[0].Exception);
    }
    catch (const wil::ResultException& re)
    {
        searchFailure = re.GetErrorCode();
    }
    catch (...) {}

    REQUIRE(searchFailure == expectedHR);
}

TEST_CASE("CompositeSource_TrackingPackageFound", "[CompositeSource]")
{
    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    CompositeWithTrackingTestSetup setup;
    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);
    auto availablePackage = setup.MakeAvailable().WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        if (request.Filters.empty())
        {
            RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);
        }
        else
        {
            REQUIRE(request.Filters.size() == 1);
            RequireSearchRequestIncludes(request.Filters, PackageMatchField::Id, MatchType::CaseInsensitive, availableID);
        }

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetInstalled()->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package)->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetLatestVersion());
}

TEST_CASE("CompositeSource_TrackingPackageFound_MetadataPopulatedFromTracking", "[CompositeSource]")
{
    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    CompositeWithTrackingTestSetup setup;
    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);
    auto availablePackage = setup.MakeAvailable().WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        if (request.Filters.empty())
        {
            RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);
        }
        else
        {
            REQUIRE(request.Filters.size() == 1);
            RequireSearchRequestIncludes(request.Filters, PackageMatchField::Id, MatchType::CaseInsensitive, availableID);
        }

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    auto manifestId = setup.Tracking->GetIndex().AddManifest(availablePackage);

    // Add test PackageVersionMetadata to be populated to InstalledVersion metadata
    setup.Tracking->GetIndex().SetMetadataByManifestId(manifestId, Repository::PackageVersionMetadata::TrackingWriteTime, "100");
    setup.Tracking->GetIndex().SetMetadataByManifestId(manifestId, Repository::PackageVersionMetadata::UserIntentArchitecture, "X86");
    setup.Tracking->GetIndex().SetMetadataByManifestId(manifestId, Repository::PackageVersionMetadata::UserIntentLocale, "en-US");
    setup.Tracking->GetIndex().SetMetadataByManifestId(manifestId, Repository::PackageVersionMetadata::InstalledArchitecture, "X86");
    setup.Tracking->GetIndex().SetMetadataByManifestId(manifestId, Repository::PackageVersionMetadata::InstalledLocale, "en-US");
    setup.Tracking->GetIndex().SetMetadataByManifestId(manifestId, Repository::PackageVersionMetadata::PinnedState, "PinnedByManifest");

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));

    auto metadata = GetInstalledVersion(result.Matches[0].Package)->GetMetadata();
    REQUIRE(metadata[Repository::PackageVersionMetadata::UserIntentArchitecture] == "X86");
    REQUIRE(metadata[Repository::PackageVersionMetadata::UserIntentLocale] == "en-US");
    REQUIRE(metadata[Repository::PackageVersionMetadata::InstalledArchitecture] == "X86");
    REQUIRE(metadata[Repository::PackageVersionMetadata::InstalledLocale] == "en-US");
    REQUIRE(metadata[Repository::PackageVersionMetadata::PinnedState] == "PinnedByManifest");
}

TEST_CASE("CompositeSource_TrackingFound_AvailableNot", "[CompositeSource]")
{
    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    CompositeWithTrackingTestSetup setup;
    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);
    auto availablePackage = setup.MakeAvailable().WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetInstalled()->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package)->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(result.Matches[0].Package->GetAvailable().empty());
}

TEST_CASE("CompositeSource_TrackingFound_AvailablePath", "[CompositeSource]")
{
    CompositeWithTrackingTestSetup setup;

    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);
    auto availablePackage = setup.MakeAvailable().WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireSearchRequestIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria(PackageMatchField::PackageFamilyName));
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Filters.size() == 1);
        RequireSearchRequestIncludes(request.Filters, PackageMatchField::Id, MatchType::CaseInsensitive, availableID);

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package));
    REQUIRE(result.Matches[0].Package->GetInstalled()->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(GetInstalledVersion(result.Matches[0].Package)->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(result.Matches[0].Package->GetAvailable().size() == 1);
    REQUIRE(result.Matches[0].Package->GetAvailable()[0]->GetLatestVersion());
}

TEST_CASE("CompositeSource_TrackingFound_NotInstalled", "[CompositeSource]")
{
    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    CompositeWithTrackingTestSetup setup;
    auto installedPackage = setup.MakeInstalled().WithPFN(pfn);
    auto availablePackage = setup.MakeAvailable().WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.empty());
}

TEST_CASE("CompositeSource_NullInstalledVersion", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(setup.MakeAvailable(), Criteria());

    // We are mostly testing to see if a null installed version causes an AV or not
    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 0);
}

TEST_CASE("CompositeSource_NullAvailableVersion", "[CompositeSource]")
{
    CompositeTestSetup setup{ CompositeSearchBehavior::AvailablePackages };
    setup.Available->Everything.Matches.emplace_back(setup.MakeInstalled(), Criteria());

    // We are mostly testing to see if a null available version causes an AV or not
    REQUIRE_THROWS_HR(setup.Search(), E_UNEXPECTED);
}

struct ExpectedResultForPinBehavior
{
    ExpectedResultForPinBehavior(bool isUpdateAvailable, std::optional<std::string> latestAvailableVersion)
        : IsUpdateAvailable(isUpdateAvailable), LatestAvailableVersion(latestAvailableVersion) {}
    ExpectedResultForPinBehavior() {}

    bool IsUpdateAvailable = false;
    std::optional<std::string> LatestAvailableVersion;
};

struct ExpectedPackageVersionKey : public PackageVersionKey
{
    ExpectedPackageVersionKey(Utility::NormalizedString sourceId, Utility::NormalizedString version, Utility::NormalizedString channel, PinType pinType) :
        PackageVersionKey(sourceId, version, channel), PinnedState(pinType) {}

    PinType PinnedState;
};

struct ExpectedResultsForPinning
{
    std::map<PinBehavior, ExpectedResultForPinBehavior> ResultsForPinBehavior;
    std::vector<ExpectedPackageVersionKey> AvailableVersions;
};

void RequireExpectedResultsWithPin(std::shared_ptr<ICompositePackage> package, const ExpectedResultsForPinning& expectedResult, std::shared_ptr<IPackageVersion> packageVersion = {})
{
    PinningData pinningData{ PinningData::Disposition::ReadOnly };
    auto availableVersions = GetAvailableVersionsForInstalledVersion(package);

    if (!packageVersion)
    {
        packageVersion = GetInstalledVersion(package);
    }

    for (const auto& entry : expectedResult.ResultsForPinBehavior)
    {
        auto pinBehavior = entry.first;
        const auto& result = entry.second;

        auto evaluator = pinningData.CreatePinStateEvaluator(pinBehavior, packageVersion);
        auto latestAvailable = evaluator.GetLatestAvailableVersionForPins(availableVersions);

        REQUIRE(evaluator.IsUpdate(latestAvailable) == result.IsUpdateAvailable);

        if (result.LatestAvailableVersion.has_value())
        {
            REQUIRE(latestAvailable);
            REQUIRE(latestAvailable->GetManifest().Version == result.LatestAvailableVersion.value());
        }
        else
        {
            REQUIRE(!latestAvailable);
        }
    }

    auto availableVersionKeys = availableVersions->GetVersionKeys();
    REQUIRE(availableVersionKeys.size() == expectedResult.AvailableVersions.size());
    for (size_t i = 0; i < availableVersionKeys.size(); ++i)
    {
        auto evaluator = pinningData.CreatePinStateEvaluator(PinBehavior::ConsiderPins, packageVersion);

        auto availableVersion = availableVersions->GetVersion(expectedResult.AvailableVersions[i]);
        REQUIRE(availableVersion);
        REQUIRE(availableVersionKeys[i].SourceId == expectedResult.AvailableVersions[i].SourceId);
        REQUIRE(availableVersionKeys[i].Version == expectedResult.AvailableVersions[i].Version);
        REQUIRE(evaluator.EvaluatePinType(availableVersion) == expectedResult.AvailableVersions[i].PinnedState);
    }
}

TEST_CASE("CompositeSource_Pinning_AvailableVersionPinned", "[CompositeSource][PinFlow]")
{
    // We use an installed package that has 3 available versions: v1.0.0, v1.0.1 and v1.1.0.
    // Installed is v1.0.1
    // We then test the 4 possible pin states (unpinned, Pinned, Blocked, Gated)
    // with the 3 possible pin search behaviors (ignore, consider, include pinned)
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    TestUserSettings userSettings;
    CompositeTestSetup setup;

    auto installedPackage = setup.MakeInstalled().WithVersion("1.0.1"sv);
    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        auto manifest1 = MakeDefaultManifest("1.0.0"sv);
        auto manifest2 = MakeDefaultManifest("1.0.1"sv);
        auto manifest3 = MakeDefaultManifest("1.1.0"sv);
        auto package = TestCompositePackage::Make(
            std::vector<Manifest::Manifest>{ manifest3, manifest2, manifest1 },
            setup.Available);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    ExpectedResultsForPinning expectedResult;
    // The result when ignoring pins is always the same
    expectedResult.ResultsForPinBehavior[PinBehavior::IgnorePins] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "1.1.0" };

    PinKey pinKey("Id", setup.Available->Details.Identifier);
    auto pinningIndex = PinningIndex::OpenOrCreateDefault();
    REQUIRE(pinningIndex);

    SECTION("Unpinned")
    {
        // If there are no pins, the result should not change if we consider them
        expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins] = expectedResult.ResultsForPinBehavior[PinBehavior::IgnorePins];
        expectedResult.ResultsForPinBehavior[PinBehavior::IncludePinned] = expectedResult.ResultsForPinBehavior[PinBehavior::IgnorePins];
        expectedResult.AvailableVersions = {
            { "AvailableTestSource1", "1.1.0", "", Pinning::PinType::Unknown },
            { "AvailableTestSource1", "1.0.1", "", Pinning::PinType::Unknown },
            { "AvailableTestSource1", "1.0.0", "", Pinning::PinType::Unknown },
        };
    }
    SECTION("Pinned")
    {
        pinningIndex->AddPin(Pin::CreatePinningPin(PinKey{ pinKey }));

        // Pinning pins are ignored with --include-pinned
        expectedResult.ResultsForPinBehavior[PinBehavior::IncludePinned] = expectedResult.ResultsForPinBehavior[PinBehavior::IgnorePins];

        expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ false, /* LatestAvailableVersion */ {} };
        expectedResult.AvailableVersions = {
            { "AvailableTestSource1", "1.1.0", "", Pinning::PinType::Pinning },
            { "AvailableTestSource1", "1.0.1", "", Pinning::PinType::Pinning },
            { "AvailableTestSource1", "1.0.0", "", Pinning::PinType::Pinning },
        };
    }
    SECTION("Blocked")
    {
        pinningIndex->AddPin(Pin::CreateBlockingPin(PinKey{ pinKey }));
        expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ false, /* LatestAvailableVersion */ {} };

        // Blocking pins are not affected by --include-pinned
        expectedResult.ResultsForPinBehavior[PinBehavior::IncludePinned] = expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins];

        expectedResult.AvailableVersions = {
            { "AvailableTestSource1", "1.1.0", "", Pinning::PinType::Blocking },
            { "AvailableTestSource1", "1.0.1", "", Pinning::PinType::Blocking },
            { "AvailableTestSource1", "1.0.0", "", Pinning::PinType::Blocking },
        };
    }
    SECTION("Gated to 1.*")
    {
        pinningIndex->AddPin(Pin::CreateGatingPin(PinKey{ pinKey }, GatedVersion{ "1.*"sv }));
        expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "1.1.0" };

        // Gating pins are not affected by --include-pinned
        expectedResult.ResultsForPinBehavior[PinBehavior::IncludePinned] = expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins];

        expectedResult.AvailableVersions = {
            { "AvailableTestSource1", "1.1.0", "", Pinning::PinType::Unknown },
            { "AvailableTestSource1", "1.0.1", "", Pinning::PinType::Unknown },
            { "AvailableTestSource1", "1.0.0", "", Pinning::PinType::Unknown },
        };
    }
    SECTION("Gated to 1.0.*")
    {
        pinningIndex->AddPin(Pin::CreateGatingPin(PinKey{ pinKey }, GatedVersion{ "1.0.*"sv }));
        expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ false, /* LatestAvailableVersion */ "1.0.1" };

        // Gating pins are not affected by --include-pinned
        expectedResult.ResultsForPinBehavior[PinBehavior::IncludePinned] = expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins];

        expectedResult.AvailableVersions = {
            { "AvailableTestSource1", "1.1.0", "", Pinning::PinType::Gating },
            { "AvailableTestSource1", "1.0.1", "", Pinning::PinType::Unknown },
            { "AvailableTestSource1", "1.0.0", "", Pinning::PinType::Unknown },
        };
    }

    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);

    RequireExpectedResultsWithPin(package, expectedResult);
}

TEST_CASE("CompositeSource_Pinning_OneSourcePinned", "[CompositeSource][PinFlow]")
{
    // We use an installed package that has 2 available sources.
    // If one of them is pinned, we should still get the updates from the other one.
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    TestUserSettings userSettings;
    CompositeTestSetup setup;

    auto installedPackage = setup.MakeInstalled().WithVersion("1.0"sv);
    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        auto package = TestCompositePackage::Make(std::vector<Manifest::Manifest>{ MakeDefaultManifest("2.0"sv) }, setup.Available);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>("SecondTestSource");
    setup.Composite.AddAvailableSource(Source{ secondAvailable });
    secondAvailable->SearchFunction = [&](const SearchRequest&)
    {
        auto package = TestCompositePackage::Make(std::vector<Manifest::Manifest>{ MakeDefaultManifest("1.1"sv) }, secondAvailable);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    {
        PinKey pinKey("Id", setup.Available->Details.Identifier);
        auto pinningIndex = PinningIndex::OpenOrCreateDefault();
        REQUIRE(pinningIndex);
        pinningIndex->AddPin(Pin::CreatePinningPin(PinKey{ pinKey }));
    }

    ExpectedResultsForPinning expectedResult;
    expectedResult.ResultsForPinBehavior[PinBehavior::IgnorePins] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "2.0" };
    expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "1.1" };
    expectedResult.ResultsForPinBehavior[PinBehavior::IncludePinned] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "2.0" };
    expectedResult.AvailableVersions = {
        { "AvailableTestSource1", "2.0", "", Pinning::PinType::Pinning },
        { "SecondTestSource", "1.1", "", Pinning::PinType::Unknown },
    };

    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    RequireExpectedResultsWithPin(package, expectedResult);
}

TEST_CASE("CompositeSource_Pinning_OneSourceGated", "[CompositeSource][PinFlow]")
{
    // We use an installed package that has 2 available sources.
    // If one of them has a gating pin, we should still get the updates from it
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    TestUserSettings userSettings;
    CompositeTestSetup setup;

    auto installedPackage = setup.MakeInstalled().WithVersion("1.0.1"sv);
    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        auto package = TestCompositePackage::Make(
            std::vector<Manifest::Manifest>{
                MakeDefaultManifest("2.0"sv),
                MakeDefaultManifest("1.2"sv),
            },
            setup.Available);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>("SecondTestSource");
    setup.Composite.AddAvailableSource(Source{ secondAvailable });
    secondAvailable->SearchFunction = [&](const SearchRequest&)
    {
        auto package = TestCompositePackage::Make(std::vector<Manifest::Manifest>{ MakeDefaultManifest("1.1"sv) }, secondAvailable);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    {
        PinKey pinKey("Id", setup.Available->Details.Identifier);
        auto pinningIndex = PinningIndex::OpenOrCreateDefault();
        REQUIRE(pinningIndex);
        pinningIndex->AddPin(Pin::CreateGatingPin(PinKey{ pinKey }, GatedVersion{ "1.*"sv }));
    }

    ExpectedResultsForPinning expectedResult;
    expectedResult.ResultsForPinBehavior[PinBehavior::IgnorePins] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "2.0" };
    expectedResult.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "1.2" };
    expectedResult.ResultsForPinBehavior[PinBehavior::IncludePinned] = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "1.2" };
    expectedResult.AvailableVersions = {
        { "AvailableTestSource1", "2.0", "", Pinning::PinType::Gating },
        { "AvailableTestSource1", "1.2", "", Pinning::PinType::Unknown },
        { "SecondTestSource", "1.1", "", Pinning::PinType::Unknown },
    };

    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    RequireExpectedResultsWithPin(package, expectedResult);
}

TEST_CASE("CompositeSource_Pinning_MultipleInstalled", "[CompositeSource][PinFlow]")
{
    // Tests the case where multiple installed packages match to a single available package.
    // If one of the two installed packages is pinned, when searching we should get
    // two Composite packages, with only one of them pinned.
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    TestUserSettings userSettings;
    
    std::string packageId = "packageId";
    std::string productCode1 = "product-code1";
    std::string productCode2 = "product-code2";

    CompositeTestSetup setup;

    // Installed packages differ in product code and version
    auto installedPackage1 = setup.MakeInstalled().WithId(productCode1).WithPC(productCode1).WithVersion("1.1"sv);
    auto installedPackage2 = setup.MakeInstalled().WithId(productCode2).WithPC(productCode2).WithVersion("1.2"sv);

    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        bool isSearchById = SearchRequestIncludes(request.Inclusions, PackageMatchField::Id, MatchType::Exact, packageId);

        SearchResult result;
        if (isSearchById || SearchRequestIncludes(request.Inclusions, PackageMatchField::ProductCode, MatchType::Exact, productCode1))
        {
            result.Matches.emplace_back(installedPackage1, Criteria(request.Inclusions[0].Field));
        }

        if (isSearchById || SearchRequestIncludes(request.Inclusions, PackageMatchField::ProductCode, MatchType::Exact, productCode2))
        {
            result.Matches.emplace_back(installedPackage2, Criteria(request.Inclusions[0].Field));
        }

        return result;
    };

    // Available package has the same ID, no product code, and different version from both the installed packages;
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithId(packageId).WithVersion("2.0"sv), Criteria());
        return result;
    };

    // We will pin the first package only
    PinKey pinKey = PinKey::GetPinKeyForInstalled(productCode1);
    auto pinningIndex = PinningIndex::OpenOrCreateDefault();
    REQUIRE(pinningIndex);

    // We will check the pinning status for both installed packages
    ExpectedResultsForPinning expectedResult1;
    ExpectedResultsForPinning expectedResult2;

    expectedResult1.ResultsForPinBehavior[PinBehavior::IgnorePins]
        = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "2.0" };

    // The second package is never pinned, so its result is always the same
    expectedResult2.ResultsForPinBehavior[PinBehavior::IgnorePins]
        = expectedResult2.ResultsForPinBehavior[PinBehavior::ConsiderPins]
        = expectedResult2.ResultsForPinBehavior[PinBehavior::IncludePinned]
        = { /* IsUpdateAvailable */ true, /* LatestAvailableVersion */ "2.0" };
    expectedResult2.AvailableVersions = {
        { "AvailableTestSource1", "2.0", "", Pinning::PinType::Unknown },
    };

    SECTION("Unpinned")
    {
        // If there are no pins, the result should not change if we consider them
        expectedResult1.ResultsForPinBehavior[PinBehavior::ConsiderPins]
            = expectedResult1.ResultsForPinBehavior[PinBehavior::IncludePinned]
            = expectedResult1.ResultsForPinBehavior[PinBehavior::IgnorePins];
        expectedResult1.AvailableVersions = {
            { "AvailableTestSource1", "2.0", "", Pinning::PinType::Unknown },
        };
    }
    SECTION("Pinned")
    {
        pinningIndex->AddPin(Pin::CreatePinningPin(PinKey{ pinKey }));

        // Pinning pins are ignored with --include-pinned
        expectedResult1.ResultsForPinBehavior[PinBehavior::IncludePinned] = expectedResult1.ResultsForPinBehavior[PinBehavior::IgnorePins];

        expectedResult1.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ false, /* LatestAvailableVersion */ {} };
        expectedResult1.AvailableVersions = {
            { "AvailableTestSource1", "2.0", "", Pinning::PinType::Pinning },
        };
    }
    SECTION("Blocked")
    {
        pinningIndex->AddPin(Pin::CreateBlockingPin(PinKey{ pinKey }));
        expectedResult1.ResultsForPinBehavior[PinBehavior::ConsiderPins] = { /* IsUpdateAvailable */ false, /* LatestAvailableVersion */ {} };

        // Blocking pins are not affected by --include-pinned
        expectedResult1.ResultsForPinBehavior[PinBehavior::IncludePinned] = expectedResult1.ResultsForPinBehavior[PinBehavior::ConsiderPins];

        expectedResult1.AvailableVersions = {
            { "AvailableTestSource1", "2.0", "", Pinning::PinType::Blocking },
        };
    }

    SearchRequest searchRequest;
    searchRequest.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, packageId);
    SearchResult result = setup.Composite.Search(searchRequest);

    REQUIRE(result.Matches.size() == 1);
    auto installedPackage = result.Matches[0].Package->GetInstalled();
    REQUIRE(installedPackage);
    auto installedVersions = installedPackage->GetVersionKeys();
    REQUIRE(installedVersions.size() == 2);

    // Here we assume that the order we return the packages in the installed source
    // search is preserved. We'll need to change it if that stops being the case.
    auto packageVersion1 = installedPackage->GetVersion(installedVersions[1]);
    REQUIRE(packageVersion1);

    auto packageVersion2 = installedPackage->GetVersion(installedVersions[0]);
    REQUIRE(packageVersion2);

    RequireExpectedResultsWithPin(result.Matches[0].Package, expectedResult1, packageVersion1);
    RequireExpectedResultsWithPin(result.Matches[0].Package, expectedResult2, packageVersion2);
}

TEST_CASE("CompositeSource_CorrelateToInstalledContainsManifestData", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        if (request.Purpose == SearchPurpose::CorrelationToInstalled)
        {
            bool expectedSearchFound = false;
            for (const auto& inclusion : request.Inclusions)
            {
                if (inclusion.Field == PackageMatchField::ProductCode && inclusion.Value == "hello")
                {
                    expectedSearchFound = true;
                    break;
                }
            }

            REQUIRE(expectedSearchFound);
        }

        SearchResult result;
        return result;
    };
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(setup.MakeAvailable().WithPC("hello"), Criteria());
        return result;
    };

    SearchRequest request;
    request.Query = RequestMatch(MatchType::Exact, "NotForEverything");
    SearchResult result = setup.Composite.Search(request);
}

TEST_CASE("CompositeSource_Respects_FeatureFlag_ManifestMayContainAdditionalSystemReferenceStrings", "[CompositeSource]")
{
    std::string id = "Special test ID";
    std::string productCode1 = "product-code1";

    CompositeTestSetup setup;
    bool productCodeSearched = false;
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
        {
            for (const auto& inclusion : request.Inclusions)
            {
                if (inclusion.Field == PackageMatchField::ProductCode)
                {
                    productCodeSearched = true;
                }
            }

            return SearchResult{};
        };
    setup.Available->SearchFunction = [&](const SearchRequest&)
        {
            SearchResult result;
            result.Matches.emplace_back(setup.MakeAvailable().WithId(id).WithPC(productCode1).HideSRS(), Criteria());
            return result;
        };

    SECTION("Feature false")
    {
        SearchRequest request;
        request.Query = RequestMatch(MatchType::Exact, "NotForEverything");
        SearchResult result = setup.Composite.Search(request);

        REQUIRE(!productCodeSearched);
    }
    SECTION("Feature true")
    {
        setup.Available->QueryFeatureFlagFunction = [](SourceFeatureFlag flag)
            {
                return (flag == SourceFeatureFlag::ManifestMayContainAdditionalSystemReferenceStrings);
            };

        SearchRequest request;
        request.Query = RequestMatch(MatchType::Exact, "NotForEverything");
        SearchResult result = setup.Composite.Search(request);

        REQUIRE(productCodeSearched);
    }
}

TEST_CASE("CompositeSource_SxS_TwoVersions_NoAvailable", "[CompositeSource][SideBySide]")
{
    std::string productCode1 = "PC1";
    std::string productCode2 = "PC2";

    CompositeTestSetup setup;
    auto availablePackage = setup.MakeAvailable();

    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion("1.0").WithPC(productCode1), Criteria());
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion("2.0").WithPC(productCode2), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 2);
}

TEST_CASE("CompositeSource_SxS_TwoVersions_DifferentAvailable", "[CompositeSource][SideBySide]")
{
    std::string productCode1 = "PC1";
    std::string productCode2 = "PC2";

    CompositeTestSetup setup;
    auto availablePackage1 = setup.MakeAvailable().ToPackage();
    auto availablePackage2 = setup.MakeAvailable().ToPackage();

    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion("1.0").WithPC(productCode1), Criteria());
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion("2.0").WithPC(productCode2), Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest& request)
        {
            SearchResult result;

            std::string productCode;
            for (const auto& item : request.Inclusions)
            {
                if (item.Field == PackageMatchField::ProductCode)
                {
                    productCode = item.Value;
                    break;
                }
            }

            if (productCode == productCode1)
            {
                result.Matches.emplace_back(availablePackage1, Criteria());
            }
            else if (productCode == productCode1)
            {
                result.Matches.emplace_back(availablePackage2, Criteria());
            }

            return result;
        };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 2);
}

TEST_CASE("CompositeSource_SxS_TwoVersions_SameAvailable", "[CompositeSource][SideBySide]")
{
    std::string version1 = "1.0";
    std::string version2 = "2.0";
    std::string productCode1 = "PC1";
    std::string productCode2 = "PC2";

    CompositeTestSetup setup;
    auto availablePackage = setup.MakeAvailable().ToPackage();

    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion(version1).WithPC(productCode1), Criteria());
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion(version2).WithPC(productCode2), Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
        {
            SearchResult result;
            result.Matches.emplace_back(availablePackage, Criteria());
            return result;
        };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    auto installedPackage = package->GetInstalled();
    REQUIRE(installedPackage);
    auto installedVersions = installedPackage->GetVersionKeys();
    REQUIRE(installedVersions.size() == 2);
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version1; }));
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version2; }));
    auto availablePackages = package->GetAvailable();
    REQUIRE(availablePackages.size() == 1);
    REQUIRE(availablePackages[0]->IsSame(availablePackage->Available[0].get()));
}

TEST_CASE("CompositeSource_SxS_ThreeVersions_SameAvailable", "[CompositeSource][SideBySide]")
{
    std::string version1 = "1.0";
    std::string version2 = "2.0";
    std::string version3 = "3.0";
    std::string productCode1 = "PC1";
    std::string productCode2 = "PC2";
    std::string productCode3 = "PC3";

    CompositeTestSetup setup;
    auto availablePackage = setup.MakeAvailable().ToPackage();

    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion(version1).WithPC(productCode1), Criteria());
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion(version2).WithPC(productCode2), Criteria());
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion(version3).WithPC(productCode3), Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
        {
            SearchResult result;
            result.Matches.emplace_back(availablePackage, Criteria());
            return result;
        };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    auto installedPackage = package->GetInstalled();
    REQUIRE(installedPackage);
    auto installedVersions = installedPackage->GetVersionKeys();
    REQUIRE(installedVersions.size() == 3);
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version1; }));
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version2; }));
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version3; }));
    auto availablePackages = package->GetAvailable();
    REQUIRE(availablePackages.size() == 1);
    REQUIRE(availablePackages[0]->IsSame(availablePackage->Available[0].get()));
}

TEST_CASE("CompositeSource_SxS_TwoVersions_SameAvailable_Tracking", "[CompositeSource][SideBySide]")
{
    std::string version1 = "1.0";
    std::string version2 = "2.0";
    std::string productCode1 = "PC1";
    std::string productCode2 = "PC2";

    CompositeWithTrackingTestSetup setup;
    auto installedPackage1 = setup.MakeInstalled().WithVersion(version1).WithPC(productCode1);
    auto availablePackage = setup.MakeAvailable().ToPackage();

    setup.Installed->Everything.Matches.emplace_back(installedPackage1, Criteria());
    setup.Installed->Everything.Matches.emplace_back(setup.MakeInstalled().WithVersion(version2).WithPC(productCode2), Criteria());
    setup.Tracking->GetIndex().AddManifest(installedPackage1);

    setup.Available->SearchFunction = [&](const SearchRequest&)
        {
            SearchResult result;
            result.Matches.emplace_back(availablePackage, Criteria());
            return result;
        };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    auto installedPackage = package->GetInstalled();
    REQUIRE(installedPackage);
    auto installedVersions = installedPackage->GetVersionKeys();
    REQUIRE(installedVersions.size() == 2);
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version1; }));
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version2; }));
    auto availablePackages = package->GetAvailable();
    REQUIRE(availablePackages.size() == 1);
    REQUIRE(availablePackages[0]->IsSame(availablePackage->Available[0].get()));
}

TEST_CASE("CompositeSource_SxS_Available_TwoVersions_SameAvailable", "[CompositeSource][SideBySide]")
{
    std::string version1 = "1.0";
    std::string version2 = "2.0";
    std::string productCode1 = "PC1";
    std::string productCode2 = "PC2";

    CompositeTestSetup setup;
    auto availablePackage = setup.MakeAvailable().ToPackage();

    setup.Installed->SearchFunction = [&](const SearchRequest&)
        {
            SearchResult result;
            result.Matches.emplace_back(setup.MakeInstalled().WithVersion(version1).WithPC(productCode1), Criteria());
            result.Matches.emplace_back(setup.MakeInstalled().WithVersion(version2).WithPC(productCode2), Criteria());
            return result;
        };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    auto installedPackage = package->GetInstalled();
    REQUIRE(installedPackage);
    auto installedVersions = installedPackage->GetVersionKeys();
    REQUIRE(installedVersions.size() == 2);
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version1; }));
    REQUIRE(std::any_of(installedVersions.begin(), installedVersions.end(), [&](const PackageVersionKey& key) { return key.Version == version2; }));
    auto availablePackages = package->GetAvailable();
    REQUIRE(availablePackages.size() == 1);
    REQUIRE(availablePackages[0]->IsSame(availablePackage->Available[0].get()));
}
