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

    ComponentTestSource(std::string_view identifier)
    {
        Details.Identifier = identifier;
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

// A helper to create the sources used by the majority of tests in this file.
struct CompositeTestSetup
{
    CompositeTestSetup(CompositeSearchBehavior behavior = CompositeSearchBehavior::Installed) : Composite("*Tests")
    {
        Installed = std::make_shared<ComponentTestSource>("InstalledTestSource1");
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

    operator std::shared_ptr<IPackage>()
    {
        if (!m_package)
        {
            if (m_isInstalled)
            {
                m_package = TestPackage::Make(m_manifest, TestPackage::MetadataMap{}, std::vector<Manifest::Manifest>(), m_source);
            }
            else
            {
                m_package = TestPackage::Make(std::vector<Manifest::Manifest>{ m_manifest }, m_source);
            }
        }

        return m_package;
    }

    operator const Manifest::Manifest& () const
    {
        return m_manifest;
    }

private:
    bool m_isInstalled;
    Manifest::Manifest m_manifest;
    std::shared_ptr<ISource> m_source;
    std::shared_ptr<TestPackage> m_package;
};

TestPackageHelper MakeInstalled()
{
    return { /* isInstalled */ true};
}

TestPackageHelper MakeAvailable(std::shared_ptr<ISource> source)
{
    return { /* isInstalled */ false, source};
}

void RequireIncludes(const std::vector<PackageMatchFilter>& filters, PackageMatchField field, MatchType type, std::optional<std::string> value = {})
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

    REQUIRE(found);
}

TEST_CASE("CompositeSource_PackageFamilyName_NotAvailable", "[CompositeSource]")
{
    // Pre-folded for easier ==
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).empty());
}

TEST_CASE("CompositeSource_PackageFamilyName_Available", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
}

TEST_CASE("CompositeSource_ProductCode_NotAvailable", "[CompositeSource]")
{
    std::string pc = "thiscouldbeapc";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPC(pc), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).empty());
}

TEST_CASE("CompositeSource_ProductCode_Available", "[CompositeSource]")
{
    std::string pc = "thiscouldbeapc";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPC(pc), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::ProductCode, MatchType::Exact, pc);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithPC(pc), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
}

TEST_CASE("CompositeSource_NameAndPublisher_Match", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::NormalizedNameAndPublisher, MatchType::Exact);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
}

TEST_CASE("CompositeSource_MultiMatch_FindsStrongMatch", "[CompositeSource]")
{
    std::string name = "MatchingName";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN("sortof_apfn"), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithId("A different ID"), Criteria(PackageMatchField::NormalizedNameAndPublisher));
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithDefaultName(name), Criteria(PackageMatchField::PackageFamilyName));
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins)->GetProperty(PackageVersionProperty::Name).get() == name);
    REQUIRE(!Version(result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins)->GetProperty(PackageVersionProperty::Version)).IsUnknown());
}

TEST_CASE("CompositeSource_MultiMatch_DoesNotFindStrongMatch", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN("sortof_apfn"), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithId("A different ID"), Criteria(PackageMatchField::NormalizedNameAndPublisher));
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithId("Another diff ID"), Criteria(PackageMatchField::NormalizedNameAndPublisher));
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 0);
}

TEST_CASE("CompositeSource_FoundByBothRootSearches", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    auto installedPackage = MakeInstalled().WithPFN(pfn);
    auto availablePackage = MakeAvailable(setup.Available).WithPFN(pfn);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
}

TEST_CASE("CompositeSource_OnlyAvailableFoundByRootSearch", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
}

TEST_CASE("CompositeSource_FoundByAvailableRootSearch_NotInstalled", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Available->Everything.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());
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
    auto installedPackage = MakeInstalled().WithPFN(pfn);
    auto availablePackage = MakeAvailable(setup.Available).WithPFN(pfn);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
    REQUIRE(result.Matches[0].MatchCriteria.Type == originalType);

    // Now make the source root search find it with a better criteria
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, PackageMatchFilter(PackageMatchField::Id, type, ""sv));

    result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
    REQUIRE(result.Matches[0].MatchCriteria.Type == type);
}

TEST_CASE("CompositePackage_PropertyFromInstalled", "[CompositeSource]")
{
    std::string id = "Special test ID";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithId(id), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetProperty(PackageProperty::Id) == id);
}

TEST_CASE("CompositePackage_PropertyFromAvailable", "[CompositeSource]")
{
    std::string id = "Special test ID";
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithId(id), Criteria());
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
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        Manifest::Manifest noChannel = MakeDefaultManifest();
        noChannel.Version = "1.0";

        Manifest::Manifest hasChannel = MakeDefaultManifest();
        hasChannel.Channel = channel;
        hasChannel.Version = "2.0";

        SearchResult result;
        result.Matches.emplace_back(TestPackage::Make(std::vector<Manifest::Manifest>{ noChannel, hasChannel }, setup.Available), Criteria());
        REQUIRE(result.Matches.back().Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 2);
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto versionKeys = result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins);
    REQUIRE(versionKeys.size() == 1);
    REQUIRE(versionKeys[0].Channel.empty());

    auto latestVersion = result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins);
    REQUIRE(latestVersion);
    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Channel).get().empty());

    REQUIRE(!result.Matches[0].Package->IsUpdateAvailable(PinBehavior::IgnorePins));
}

TEST_CASE("CompositePackage_AvailableVersions_NoChannelFilteredOut", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string channel = "Channel";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn).WithChannel(channel), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        Manifest::Manifest noChannel = MakeDefaultManifest();
        noChannel.Version = "1.0";

        Manifest::Manifest hasChannel = MakeDefaultManifest();
        hasChannel.Channel = channel;
        hasChannel.Version = "2.0";

        SearchResult result;
        result.Matches.emplace_back(TestPackage::Make(std::vector<Manifest::Manifest>{ noChannel, hasChannel }, setup.Available), Criteria());
        REQUIRE(result.Matches.back().Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 2);
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto versionKeys = result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins);
    REQUIRE(versionKeys.size() == 1);
    REQUIRE(versionKeys[0].Channel == channel);

    auto latestVersion = result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins);
    REQUIRE(latestVersion);
    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Channel).get() == channel);

    REQUIRE(result.Matches[0].Package->IsUpdateAvailable(PinBehavior::IgnorePins));
}

TEST_CASE("CompositeSource_MultipleAvailableSources_MatchAll", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string firstName = "Name1";
    std::string secondName = "Name2";

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(Source{ secondAvailable });

    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithDefaultName(firstName), Criteria());
        return result;
    };

    secondAvailable->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(secondAvailable).WithDefaultName(secondName), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 2);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins)->GetProperty(PackageVersionProperty::Name).get() == firstName);
}

TEST_CASE("CompositeSource_MultipleAvailableSources_MatchSecond", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string firstName = "Name1";
    std::string secondName = "Name2";

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(Source{ secondAvailable });

    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());

    secondAvailable->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithDefaultName(secondName), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins)->GetProperty(PackageVersionProperty::Name).get() == secondName);
}

TEST_CASE("CompositeSource_MultipleAvailableSources_ReverseMatchBoth", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    auto installedPackage = MakeInstalled().WithPFN(pfn);

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(Source{ secondAvailable });

    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());
    secondAvailable->Everything.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys(PinBehavior::IgnorePins).size() == 1);
}

TEST_CASE("CompositeSource_IsSame", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN("sortof_apfn"), Criteria());
    setup.Available->Everything.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN("sortof_apfn"), Criteria());

    SearchResult result1 = setup.Search();
    REQUIRE(result1.Matches.size() == 1);

    SearchResult result2 = setup.Search();
    REQUIRE(result2.Matches.size() == 1);

    REQUIRE(result1.Matches[0].Package->IsSame(result2.Matches[0].Package.get()));
}

TEST_CASE("CompositeSource_AvailableSearchFailure", "[CompositeSource]")
{
    HRESULT expectedHR = E_BLUETOOTH_ATT_ATTRIBUTE_NOT_FOUND;
    std::string pfn = "sortof_apfn";

    std::shared_ptr<ComponentTestSource> AvailableSucceeds = std::make_shared<ComponentTestSource>();
    AvailableSucceeds->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(MakeAvailable({}).WithPFN(pfn), Criteria());
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

    auto pfns = result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins)->GetMultiProperty(PackageVersionMultiProperty::PackageFamilyName);
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
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled().WithPFN(pfn), Criteria());
    setup.Available->Everything.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());

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
        result.Matches.emplace_back(MakeAvailable(setup.Available).WithPFN(pfn), Criteria());
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
    auto installedPackage = MakeInstalled().WithPFN(pfn);
    auto availablePackage = MakeAvailable(setup.Available).WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        if (request.Filters.empty())
        {
            RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);
        }
        else
        {
            REQUIRE(request.Filters.size() == 1);
            RequireIncludes(request.Filters, PackageMatchField::Id, MatchType::CaseInsensitive, availableID);
        }

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetInstalledVersion()->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins));
}

TEST_CASE("CompositeSource_TrackingPackageFound_MetadataPopulatedFromTracking", "[CompositeSource]")
{
    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    CompositeWithTrackingTestSetup setup;
    auto installedPackage = MakeInstalled().WithPFN(pfn);
    auto availablePackage = MakeAvailable(setup.Available).WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        if (request.Filters.empty())
        {
            RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);
        }
        else
        {
            REQUIRE(request.Filters.size() == 1);
            RequireIncludes(request.Filters, PackageMatchField::Id, MatchType::CaseInsensitive, availableID);
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
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());

    auto metadata = result.Matches[0].Package->GetInstalledVersion()->GetMetadata();
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
    auto installedPackage = MakeInstalled().WithPFN(pfn);
    auto availablePackage = MakeAvailable(setup.Available).WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetInstalledVersion()->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(!result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins));
}

TEST_CASE("CompositeSource_TrackingFound_AvailablePath", "[CompositeSource]")
{
    CompositeWithTrackingTestSetup setup;

    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    auto installedPackage = MakeInstalled().WithPFN(pfn);
    auto availablePackage = MakeAvailable(setup.Available).WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        RequireIncludes(request.Inclusions, PackageMatchField::PackageFamilyName, MatchType::Exact, pfn);

        SearchResult result;
        result.Matches.emplace_back(installedPackage, Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Filters.size() == 1);
        RequireIncludes(request.Filters, PackageMatchField::Id, MatchType::CaseInsensitive, availableID);

        SearchResult result;
        result.Matches.emplace_back(availablePackage, Criteria());
        return result;
    };

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetInstalledVersion()->GetSource().GetIdentifier() == setup.Available->Details.Identifier);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion(PinBehavior::IgnorePins));
}

TEST_CASE("CompositeSource_TrackingFound_NotInstalled", "[CompositeSource]")
{
    std::string availableID = "Available.ID";
    std::string pfn = "sortof_apfn";

    CompositeWithTrackingTestSetup setup;
    auto installedPackage = MakeInstalled().WithPFN(pfn);
    auto availablePackage = MakeAvailable(setup.Available).WithPFN(pfn).WithId(availableID).WithDefaultName(s_Everything_Query);

    setup.Available->Everything.Matches.emplace_back(availablePackage, Criteria());

    setup.Tracking->GetIndex().AddManifest(availablePackage);

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.empty());
}

TEST_CASE("CompositeSource_NullInstalledVersion", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeAvailable(setup.Available), Criteria());

    // We are mostly testing to see if a null installed version causes an AV or not
    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 0);
}

TEST_CASE("CompositeSource_NullAvailableVersion", "[CompositeSource]")
{
    CompositeTestSetup setup{ CompositeSearchBehavior::AvailablePackages };
    setup.Available->Everything.Matches.emplace_back(MakeInstalled(), Criteria());

    // We are mostly testing to see if a null available version causes an AV or not
    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 1);
}

struct ExpectedResultWithPin
{
    bool IsUpdateAvailable;
    std::optional<std::string> LatestAvailableVersion;
    std::vector<std::string> AvailableVersions;
    std::vector<std::string> UnavailableVersions;
};

void RequireExpectedResultsWithPin(std::shared_ptr<IPackage> package, PinBehavior pinBehavior, const ExpectedResultWithPin& expectedResult)
{
    REQUIRE(package->IsUpdateAvailable(pinBehavior) == expectedResult.IsUpdateAvailable);

    auto latestAvailable = package->GetLatestAvailableVersion(pinBehavior);
    if (expectedResult.LatestAvailableVersion.has_value())
    {
        REQUIRE(latestAvailable);
        REQUIRE(latestAvailable->GetManifest().Version == expectedResult.LatestAvailableVersion.value());
    }
    else
    {
        REQUIRE(!latestAvailable);
    }

    auto availableVersionKeys = package->GetAvailableVersionKeys(pinBehavior);
    REQUIRE(availableVersionKeys.size() == expectedResult.AvailableVersions.size());
    for (size_t i = 0; i < availableVersionKeys.size(); ++i)
    {
        REQUIRE(availableVersionKeys[i].Version == expectedResult.AvailableVersions[i]);
    }

    for (const auto& expectedAvailableVersion : expectedResult.AvailableVersions)
    {
        REQUIRE(package->GetAvailableVersion({ "", expectedAvailableVersion, "" }, pinBehavior));
    }

    for (const auto& expectedUnavailableVersion : expectedResult.UnavailableVersions)
    {
        REQUIRE(!(package->GetAvailableVersion({ "", expectedUnavailableVersion, "" }, pinBehavior)));
    }
}

TEST_CASE("CompositeSource_PinnedAvailable", "[CompositeSource][PinFlow]")
{
    // We use an installed package that has 3 available versions: v1.0.0, v1.0.1 and v1.1.0.
    // Installed is v1.0.1
    // We then test the 4 possible pin states (unpinned, Pinned, Blocked, Gated)
    // with the 3 possible pin search behaviors (ignore, consider, include pinned)
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    TestUserSettings userSettings;
    userSettings.Set<Settings::Setting::EFPinning>(true);

    CompositeTestSetup setup;

    auto installedPackage = TestPackage::Make(MakeDefaultManifest("1.0.1"sv), TestPackage::MetadataMap{});
    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        auto manifest1 = MakeDefaultManifest("1.0.0"sv);
        auto manifest2 = MakeDefaultManifest("1.0.1"sv);
        auto manifest3 = MakeDefaultManifest("1.1.0"sv);
        auto package = TestPackage::Make(
            std::vector<Manifest::Manifest>{ manifest1, manifest2, manifest3 },
            setup.Available);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    // The result when ignoring pins is always the same
    ExpectedResultWithPin expectedResult_ignorePins;
    expectedResult_ignorePins.IsUpdateAvailable = true;
    expectedResult_ignorePins.LatestAvailableVersion = "1.1.0";
    expectedResult_ignorePins.AvailableVersions = { "1.1.0", "1.0.1", "1.0.0" };
    expectedResult_ignorePins.UnavailableVersions = {};

    ExpectedResultWithPin expectedResult_includePinned;
    ExpectedResultWithPin expectedResult_considerPins;

    PinKey pinKey("Id", setup.Available->Details.Identifier);
    PinningIndex pinningIndex = PinningIndex::OpenOrCreateDefault();

    SECTION("Unpinned")
    {
        // If there are no pins, the result should not change if we consider them
        expectedResult_considerPins = expectedResult_ignorePins;
        expectedResult_includePinned = expectedResult_ignorePins;
    }
    SECTION("Pinned")
    {
        pinningIndex.AddPin(Pin::CreatePinningPin(PinKey{ pinKey }));

        // Pinning pins are ignored with --include-pinned
        expectedResult_includePinned = expectedResult_ignorePins;

        expectedResult_considerPins.IsUpdateAvailable = false;
        expectedResult_considerPins.LatestAvailableVersion = {};
        expectedResult_considerPins.AvailableVersions = {};
        expectedResult_considerPins.UnavailableVersions = { "1.1.0", "1.0.1", "1.0.0" };
    }
    SECTION("Blocked")
    {
        pinningIndex.AddPin(Pin::CreateBlockingPin(PinKey{ pinKey }));

        expectedResult_considerPins.IsUpdateAvailable = false;
        expectedResult_considerPins.LatestAvailableVersion = {};
        expectedResult_considerPins.AvailableVersions = {};
        expectedResult_considerPins.UnavailableVersions = { "1.1.0", "1.0.1", "1.0.0" };

        // Blocking pins are not affected by --include-pinned
        expectedResult_includePinned = expectedResult_considerPins;
    }
    SECTION("Gated to 1.*")
    {
        pinningIndex.AddPin(Pin::CreateGatingPin(PinKey{ pinKey }, GatedVersion{ "1.*"sv }));

        expectedResult_considerPins.IsUpdateAvailable = true;
        expectedResult_considerPins.LatestAvailableVersion = "1.1.0";
        expectedResult_considerPins.AvailableVersions = { "1.1.0", "1.0.1", "1.0.0" };
        expectedResult_considerPins.UnavailableVersions = {};

        // Gating pins are not affected by --include-pinned
        expectedResult_includePinned = expectedResult_considerPins;
    }
    SECTION("Gated to 1.0.*")
    {
        pinningIndex.AddPin(Pin::CreateGatingPin(PinKey{ pinKey }, GatedVersion{ "1.0.*"sv }));

        expectedResult_considerPins.IsUpdateAvailable = false;
        expectedResult_considerPins.LatestAvailableVersion = "1.0.1";
        expectedResult_considerPins.AvailableVersions = { "1.0.1", "1.0.0" };
        expectedResult_considerPins.UnavailableVersions = { "1.1.0" };

        // Gating pins are not affected by --include-pinned
        expectedResult_includePinned = expectedResult_considerPins;
    }

    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);

    RequireExpectedResultsWithPin(package, PinBehavior::IgnorePins, expectedResult_ignorePins);
    RequireExpectedResultsWithPin(package, PinBehavior::IncludePinned, expectedResult_includePinned);
    RequireExpectedResultsWithPin(package, PinBehavior::ConsiderPins, expectedResult_considerPins);
}

TEST_CASE("CompositeSource_OneSourcePinned", "[CompositeSource][PinFlow]")
{
    // We use an installed package that has 2 available sources.
    // If one of them is pinned, we should still get the updates from the other one.
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    TestUserSettings userSettings;
    userSettings.Set<Settings::Setting::EFPinning>(true);

    CompositeTestSetup setup;

    auto installedPackage = TestPackage::Make(MakeDefaultManifest("1.0"sv), TestPackage::MetadataMap{});
    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        auto package = TestPackage::Make(std::vector<Manifest::Manifest>{ MakeDefaultManifest("2.0"sv) }, setup.Available);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>("SecondTestSource");
    setup.Composite.AddAvailableSource(Source{ secondAvailable });
    secondAvailable->SearchFunction = [&](const SearchRequest&)
    {
        auto package = TestPackage::Make(std::vector<Manifest::Manifest>{ MakeDefaultManifest("1.1"sv) }, secondAvailable);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    {
        PinKey pinKey("Id", setup.Available->Details.Identifier);
        PinningIndex pinningIndex = PinningIndex::OpenOrCreateDefault();
        pinningIndex.AddPin(Pin::CreatePinningPin(PinKey{ pinKey }));
    }

    PinBehavior pinBehavior = PinBehavior::IgnorePins;
    ExpectedResultWithPin expectedResult;
    SECTION("Ignore pins")
    {
        pinBehavior = PinBehavior::IgnorePins;
        expectedResult.IsUpdateAvailable = true;
        expectedResult.LatestAvailableVersion = "2.0";
        expectedResult.AvailableVersions = { "2.0", "1.1" };
        expectedResult.UnavailableVersions = {};
    }
    SECTION("Include pinned")
    {
        pinBehavior = PinBehavior::IncludePinned;
        expectedResult.IsUpdateAvailable = true;
        expectedResult.LatestAvailableVersion = "2.0";
        expectedResult.AvailableVersions = { "2.0", "1.1" };
        expectedResult.UnavailableVersions = {};
    }
    SECTION("Consider pins")
    {
        pinBehavior = PinBehavior::ConsiderPins;
        expectedResult.IsUpdateAvailable = true;
        expectedResult.LatestAvailableVersion = "1.1";
        expectedResult.AvailableVersions = { "1.1" };
        expectedResult.UnavailableVersions = { "2.0" };
    }

    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    RequireExpectedResultsWithPin(package, pinBehavior, expectedResult);
}

TEST_CASE("CompositeSource_OneSourceGated", "[CompositeSource][PinFlow]")
{
    // We use an installed package that has 2 available sources.
    // If one of them has a gating pin, we should still get the updates from it
    TempFile indexFile("pinningIndex", ".db");
    TestHook::SetPinningIndex_Override pinningIndexOverride(indexFile.GetPath());

    TestUserSettings userSettings;
    userSettings.Set<Settings::Setting::EFPinning>(true);

    CompositeTestSetup setup;

    auto installedPackage = TestPackage::Make(MakeDefaultManifest("1.0"sv), TestPackage::MetadataMap{});
    setup.Installed->Everything.Matches.emplace_back(installedPackage, Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        auto package = TestPackage::Make(
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
        auto package = TestPackage::Make(std::vector<Manifest::Manifest>{ MakeDefaultManifest("1.1"sv) }, secondAvailable);

        SearchResult result;
        result.Matches.emplace_back(package, Criteria());
        return result;
    };

    {
        PinKey pinKey("Id", setup.Available->Details.Identifier);
        PinningIndex pinningIndex = PinningIndex::OpenOrCreateDefault();
        pinningIndex.AddPin(Pin::CreateGatingPin(PinKey{ pinKey }, GatedVersion{ "1.*"sv }));
    }

    PinBehavior pinBehavior = PinBehavior::IgnorePins;
    ExpectedResultWithPin expectedResult;
    SECTION("Ignore pins")
    {
        pinBehavior = PinBehavior::IgnorePins;
        expectedResult.IsUpdateAvailable = true;
        expectedResult.LatestAvailableVersion = "2.0";
        expectedResult.AvailableVersions = { "2.0", "1.2", "1.1"};
        expectedResult.UnavailableVersions = {};
    }
    SECTION("Include pinned")
    {
        pinBehavior = PinBehavior::IncludePinned;
        expectedResult.IsUpdateAvailable = true;
        expectedResult.LatestAvailableVersion = "1.2";
        expectedResult.AvailableVersions = { "1.2", "1.1" };
        expectedResult.UnavailableVersions = { "2.0" };
    }
    SECTION("Consider pins")
    {
        pinBehavior = PinBehavior::ConsiderPins;
        expectedResult.IsUpdateAvailable = true;
        expectedResult.LatestAvailableVersion = "1.2";
        expectedResult.AvailableVersions = { "1.2", "1.1"};
        expectedResult.UnavailableVersions = { "2.0" };
    }

    SearchResult result = setup.Search();
    REQUIRE(result.Matches.size() == 1);
    auto package = result.Matches[0].Package;
    REQUIRE(package);
    RequireExpectedResultsWithPin(package, pinBehavior, expectedResult);
}