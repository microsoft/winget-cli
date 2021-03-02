// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include <CompositeSource.h>

using namespace std::string_literals;
using namespace std::string_view_literals;
using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

constexpr std::string_view s_Everything_Query = "everything"sv;

// A test source that has two modes:
//  1. A request that IsForEverything returns the stored result. This models the
//      incoming search request to a CompositeSource.
//  2. A request that is not for everything invokes TestSource::SearchFunction to
//      enable verification of expectations.
struct ComponentTestSource : public TestSource
{
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
    CompositeTestSetup() : Composite("*Tests")
    {
        Installed = std::make_shared<ComponentTestSource>();
        Available = std::make_shared<ComponentTestSource>();
        Composite.SetInstalledSource(Installed);
        Composite.AddAvailableSource(Available);
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

// A helper to make matches.
struct Criteria : public PackageMatchFilter
{
    Criteria() : PackageMatchFilter(PackageMatchField::Id, MatchType::Wildcard, ""sv) {}
};

Manifest::Manifest MakeDefaultManifest()
{
    Manifest::Manifest result;

    result.Id = "Id";
    result.DefaultLocalization.Add<Manifest::Localization::PackageName>("Name");
    result.DefaultLocalization.Add<Manifest::Localization::Publisher>("Publisher");
    result.Version = "1.0";
    result.Installers.push_back({});

    return result;
}

std::shared_ptr<TestPackage> MakeInstalled(std::function<void(Manifest::Manifest&)> op)
{
    Manifest::Manifest manifest = MakeDefaultManifest();
    op(manifest);
    return TestPackage::Make(manifest, TestPackage::MetadataMap{});
}

std::shared_ptr<TestPackage> MakeAvailable(std::function<void(Manifest::Manifest&)> op)
{
    Manifest::Manifest manifest = MakeDefaultManifest();
    op(manifest);
    return TestPackage::Make(std::vector<Manifest::Manifest>{ manifest });
}

std::function<void(Manifest::Manifest&)> WithPFN(const std::string& pfn)
{
    return [pfn](Manifest::Manifest& m) { m.Installers[0].PackageFamilyName = pfn; };
}

std::function<void(Manifest::Manifest&)> WithPC(const std::string& pc)
{
    return [pc](Manifest::Manifest& m) { m.Installers[0].ProductCode = pc; };
}

TEST_CASE("CompositeSource_PackageFamilyName_NotAvailable", "[CompositeSource]")
{
    // Pre-folded for easier ==
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().empty());
}

TEST_CASE("CompositeSource_PackageFamilyName_Available", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_ProductCode_NotAvailable", "[CompositeSource]")
{
    std::string pc = "thiscouldbeapc";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPC(pc)), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().empty());
}

TEST_CASE("CompositeSource_ProductCode_Available", "[CompositeSource]")
{
    std::string pc = "thiscouldbeapc";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPC(pc)), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pc);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPC(pc)), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_MultiMatch_FindsId", "[CompositeSource]")
{
    std::string name = "MatchingName";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN("sortof_apfn")), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(MakeAvailable([](Manifest::Manifest& m) { m.Id = "A different ID"; }), Criteria());
        result.Matches.emplace_back(MakeAvailable([&](Manifest::Manifest& m) { m.DefaultLocalization.Add<Manifest::Localization::PackageName>(name); }), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion()->GetProperty(PackageVersionProperty::Name).get() == name);
    REQUIRE(!Version(result.Matches[0].Package->GetLatestAvailableVersion()->GetProperty(PackageVersionProperty::Version)).IsUnknown());
}

TEST_CASE("CompositeSource_MultiMatch_DoesNotFindId", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN("sortof_apfn")), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(MakeAvailable([](Manifest::Manifest& m) { m.Id = "A different ID"; }), Criteria());
        result.Matches.emplace_back(MakeAvailable([&](Manifest::Manifest& m) { m.Id = "Another diff ID"; }), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
    REQUIRE(Version(result.Matches[0].Package->GetLatestAvailableVersion()->GetProperty(PackageVersionProperty::Version)).IsUnknown());
}

TEST_CASE("CompositeSource_FoundByBothRootSearches", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_OnlyAvailableFoundByRootSearch", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_FoundByAvailableRootSearch_NotInstalled", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Available->Everything.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
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
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].MatchCriteria.Type == originalType);

    // Now make the source root search find it with a better criteria
    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), PackageMatchFilter(PackageMatchField::Id, type, ""sv));

    result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].MatchCriteria.Type == type);
}

TEST_CASE("CompositePackage_PropertyFromInstalled", "[CompositeSource]")
{
    std::string id = "Special test ID";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled([&](Manifest::Manifest& m) { m.Id = id; }), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetProperty(PackageProperty::Id) == id);
}

TEST_CASE("CompositePackage_PropertyFromAvailable", "[CompositeSource]")
{
    std::string id = "Special test ID";
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        SearchResult result;
        result.Matches.emplace_back(MakeAvailable([&](Manifest::Manifest& m) { m.Id = id; }), Criteria());
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
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        Manifest::Manifest noChannel = MakeDefaultManifest();
        noChannel.Version = "1.0";

        Manifest::Manifest hasChannel = MakeDefaultManifest();
        hasChannel.Channel = channel;
        hasChannel.Version = "2.0";

        SearchResult result;
        result.Matches.emplace_back(TestPackage::Make(std::vector<Manifest::Manifest>{ noChannel, hasChannel }), Criteria());
        REQUIRE(result.Matches.back().Package->GetAvailableVersionKeys().size() == 2);
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto versionKeys = result.Matches[0].Package->GetAvailableVersionKeys();
    REQUIRE(versionKeys.size() == 1);
    REQUIRE(versionKeys[0].Channel.empty());

    auto latestVersion = result.Matches[0].Package->GetLatestAvailableVersion();
    REQUIRE(latestVersion);
    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Channel).get().empty());

    REQUIRE(!result.Matches[0].Package->IsUpdateAvailable());
}

TEST_CASE("CompositePackage_AvailableVersions_NoChannelFilteredOut", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string channel = "Channel";

    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled([&](Manifest::Manifest& m) { m.Installers[0].PackageFamilyName = pfn; m.Channel = channel; }), Criteria());
    setup.Available->SearchFunction = [&](const SearchRequest&)
    {
        Manifest::Manifest noChannel = MakeDefaultManifest();
        noChannel.Version = "1.0";

        Manifest::Manifest hasChannel = MakeDefaultManifest();
        hasChannel.Channel = channel;
        hasChannel.Version = "2.0";

        SearchResult result;
        result.Matches.emplace_back(TestPackage::Make(std::vector<Manifest::Manifest>{ noChannel, hasChannel }), Criteria());
        REQUIRE(result.Matches.back().Package->GetAvailableVersionKeys().size() == 2);
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    auto versionKeys = result.Matches[0].Package->GetAvailableVersionKeys();
    REQUIRE(versionKeys.size() == 1);
    REQUIRE(versionKeys[0].Channel == channel);

    auto latestVersion = result.Matches[0].Package->GetLatestAvailableVersion();
    REQUIRE(latestVersion);
    REQUIRE(latestVersion->GetProperty(PackageVersionProperty::Channel).get() == channel);

    REQUIRE(result.Matches[0].Package->IsUpdateAvailable());
}

TEST_CASE("CompositeSource_MultipleAvailableSources_MatchFirst", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string firstName = "Name1";
    std::string secondName = "Name2";

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(secondAvailable);

    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());

    setup.Available->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable([&](Manifest::Manifest& m) { m.DefaultLocalization.Add<Manifest::Localization::PackageName>(firstName); }), Criteria());
        return result;
    };

    secondAvailable->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable([&](Manifest::Manifest& m) { m.DefaultLocalization.Add<Manifest::Localization::PackageName>(secondName); }), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion()->GetProperty(PackageVersionProperty::Name).get() == firstName);
}

TEST_CASE("CompositeSource_MultipleAvailableSources_MatchSecond", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";
    std::string firstName = "Name1";
    std::string secondName = "Name2";

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(secondAvailable);

    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());

    secondAvailable->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeAvailable([&](Manifest::Manifest& m) { m.DefaultLocalization.Add<Manifest::Localization::PackageName>(secondName); }), Criteria());
        return result;
    };

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
    REQUIRE(result.Matches[0].Package->GetLatestAvailableVersion()->GetProperty(PackageVersionProperty::Name).get() == secondName);
}

TEST_CASE("CompositeSource_MultipleAvailableSources_ReverseMatchBoth", "[CompositeSource]")
{
    std::string pfn = "sortof_apfn";

    CompositeTestSetup setup;
    std::shared_ptr<ComponentTestSource> secondAvailable = std::make_shared<ComponentTestSource>();
    setup.Composite.AddAvailableSource(secondAvailable);

    setup.Installed->SearchFunction = [&](const SearchRequest& request)
    {
        REQUIRE(request.Inclusions.size() == 1);
        REQUIRE(request.Inclusions[0].Value == pfn);

        SearchResult result;
        result.Matches.emplace_back(MakeInstalled(WithPFN(pfn)), Criteria());
        return result;
    };

    setup.Available->Everything.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());
    secondAvailable->Everything.Matches.emplace_back(MakeAvailable(WithPFN(pfn)), Criteria());

    SearchResult result = setup.Search();

    REQUIRE(result.Matches.size() == 1);
    REQUIRE(result.Matches[0].Package->GetInstalledVersion());
    REQUIRE(result.Matches[0].Package->GetAvailableVersionKeys().size() == 1);
}

TEST_CASE("CompositeSource_IsSame", "[CompositeSource]")
{
    CompositeTestSetup setup;
    setup.Installed->Everything.Matches.emplace_back(MakeInstalled(WithPFN("sortof_apfn")), Criteria());
    setup.Available->Everything.Matches.emplace_back(MakeAvailable(WithPFN("sortof_apfn")), Criteria());

    SearchResult result1 = setup.Search();
    REQUIRE(result1.Matches.size() == 1);

    SearchResult result2 = setup.Search();
    REQUIRE(result2.Matches.size() == 1);

    REQUIRE(result1.Matches[0].Package->IsSame(result2.Matches[0].Package.get()));
}
