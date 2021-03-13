// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include "TestHooks.h"
#include <Workflows/WorkflowBase.h>
#include <Workflows/InstallFlow.h>
#include <winget/Manifest.h>
#include <Microsoft/PredefinedInstalledSourceFactory.h>

using namespace TestCommon;
using namespace AppInstaller;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Logging;

struct TestTelemetry : public TelemetryTraceLogger
{
    void LogSuccessfulInstallARPChange(
        std::string_view sourceIdentifier,
        std::string_view packageIdentifier,
        std::string_view packageVersion,
        std::string_view packageChannel,
        size_t changesToARP,
        size_t matchesInARP,
        size_t countOfIntersectionOfChangesAndMatches,
        std::string_view arpName,
        std::string_view arpVersion,
        std::string_view arpPublisher,
        std::string_view arpLanguage) const noexcept override
    {
        WasLogSuccessfulInstallARPChangeCalled = true;
        if (OnLogSuccessfulInstallARPChange)
        {
            OnLogSuccessfulInstallARPChange(
                sourceIdentifier, packageIdentifier, packageVersion, packageChannel,
                changesToARP, matchesInARP, countOfIntersectionOfChangesAndMatches,
                arpName, arpVersion, arpPublisher, arpLanguage);
        }
    }

    std::function<void(
        std::string_view, std::string_view, std::string_view, std::string_view,
        size_t, size_t, size_t,
        std::string_view, std::string_view, std::string_view, std::string_view)> OnLogSuccessfulInstallARPChange;

    mutable bool WasLogSuccessfulInstallARPChangeCalled = false;
};

struct TestContext : public Context
{
    TestContext(Manifest::InstallerTypeEnum installerType = Manifest::InstallerTypeEnum::Exe) :
        Context(OStream, IStream), SourceFactory([this](const SourceDetails&) { return Source; })
    {
        // Put installer in to control whether arp change code cares to run
        Manifest::ManifestInstaller installer;
        installer.InstallerType = installerType;
        Add<Data::Installer>(std::move(installer));

        // Put in an empty manifest by default
        Manifest::Manifest manifest;
        manifest.Id = "Installing.Id";
        manifest.Version = "Installing.Version";
        manifest.Channel = "Installing.Channel";
        manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>("Installing.Name");
        Add<Data::Manifest>(std::move(manifest));

        // Set up logger to intercept event
        Logger = std::make_shared<TestTelemetry>();
        TestHook_SetTelemetryOverride(Logger);

        Logger->OnLogSuccessfulInstallARPChange = [this](
            std::string_view sourceIdentifier,
            std::string_view packageIdentifier,
            std::string_view packageVersion,
            std::string_view packageChannel,
            size_t changesToARP,
            size_t matchesInARP,
            size_t countOfIntersectionOfChangesAndMatches,
            std::string_view arpName,
            std::string_view arpVersion,
            std::string_view arpPublisher,
            std::string_view arpLanguage)
        {
            SourceIdentifier = sourceIdentifier;
            PackageIdentifier = packageIdentifier;
            PackageVersion = packageVersion;
            PackageChannel = packageChannel;
            ChangesToARP = changesToARP;
            MatchesInARP = matchesInARP;
            CountOfIntersectionOfChangesAndMatches = countOfIntersectionOfChangesAndMatches;
            ARPName = arpName;
            ARPVersion = arpVersion;
            ARPPublisher = arpPublisher;
            ARPLanguage = arpLanguage;
        };

        // Inject our source
        TestHook_SetSourceFactoryOverride(std::string{ Repository::Microsoft::PredefinedInstalledSourceFactory::Type() }, SourceFactory);

        Source = std::make_shared<TestSource>();
        Source->SearchFunction = [&](const SearchRequest& request)
        {
            return request.IsForEverything() ? EverythingResult : MatchResult;
        };

        // The package version is used to get the source identifier
        Add<Data::PackageVersion>(TestPackageVersion::Make(Get<Data::Manifest>(), Source));

        // Populate everything result with a few items
        AddEverythingResult("Id1", "Name1", "Publisher1", "1.0");
        AddEverythingResult("Id2", "Name2", "Publisher2", "2.0");
    }

    ~TestContext()
    {
        TestHook_ClearSourceFactoryOverrides();
        TestHook_SetTelemetryOverride({});
    }

    void AddEverythingResult(std::string_view id, std::string_view name, std::string_view publisher, std::string_view version)
    {
        AddResult(EverythingResult, id, name, publisher, version);
    }

    void AddMatchResult(std::string_view id, std::string_view name, std::string_view publisher, std::string_view version)
    {
        AddResult(MatchResult, id, name, publisher, version);
    }

    void ExpectEvent(size_t arpChanges, size_t matches, size_t overlap, IPackage* arpEntry = nullptr)
    {
        REQUIRE(Logger->WasLogSuccessfulInstallARPChangeCalled);

        const auto& manifest = Get<Data::Manifest>();

        REQUIRE(Source->GetIdentifier() == SourceIdentifier);
        REQUIRE(manifest.Id == PackageIdentifier);
        REQUIRE(manifest.Version == PackageVersion);
        REQUIRE(manifest.Channel == PackageChannel);
        REQUIRE(arpChanges == ChangesToARP);
        REQUIRE(matches == MatchesInARP);
        REQUIRE(overlap == CountOfIntersectionOfChangesAndMatches);

        if (arpEntry)
        {
            auto version = arpEntry->GetInstalledVersion();
            REQUIRE(version->GetProperty(PackageVersionProperty::Name) == ARPName);
            REQUIRE(version->GetProperty(PackageVersionProperty::Version) == ARPVersion);

            auto metadata = version->GetMetadata();
            REQUIRE(metadata[PackageVersionMetadata::Publisher] == ARPPublisher);
            REQUIRE(metadata[PackageVersionMetadata::Locale] == ARPLanguage);
        }
        else
        {
            REQUIRE(ARPName.empty());
            REQUIRE(ARPVersion.empty());
            REQUIRE(ARPPublisher.empty());
            REQUIRE(ARPLanguage.empty());
        }
    }

    std::ostringstream OStream;
    std::istringstream IStream;
    std::shared_ptr<TestTelemetry> Logger;
    TestSourceFactory SourceFactory;
    std::shared_ptr<TestSource> Source;
    SearchResult EverythingResult;
    SearchResult MatchResult;

    // EventData
    std::string SourceIdentifier;
    std::string PackageIdentifier;
    std::string PackageVersion;
    std::string PackageChannel;
    size_t ChangesToARP;
    size_t MatchesInARP;
    size_t CountOfIntersectionOfChangesAndMatches;
    std::string ARPName;
    std::string ARPVersion;
    std::string ARPPublisher;
    std::string ARPLanguage;

    private:
        void AddResult(SearchResult& result, std::string_view id, std::string_view name, std::string_view publisher, std::string_view version)
        {
            PackageMatchFilter defaultFilter{ PackageMatchField::Id, MatchType::Exact };
            Manifest::Manifest manifest;

            manifest.Id = id;
            manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(name);
            manifest.DefaultLocalization.Add<Manifest::Localization::Publisher>(publisher);
            manifest.Version = version;
            manifest.Installers.push_back({});

            TestPackage::MetadataMap metadata;
            metadata[PackageVersionMetadata::Publisher] = publisher;

            result.Matches.emplace_back(TestPackage::Make(manifest, std::move(metadata), std::vector<Manifest::Manifest>{}, Source), defaultFilter);
        }
};


TEST_CASE("ARPChanges_MSIX_Ignored", "[ARPChanges][workflow]")
{
    TestContext context(Manifest::InstallerTypeEnum::Msix);

    context << SnapshotARPEntries;

    REQUIRE(!context.Contains(Data::ARPSnapshot));

    context << ReportARPChanges;

    REQUIRE(!context.Logger->WasLogSuccessfulInstallARPChangeCalled);
}

TEST_CASE("ARPChanges_CheckSnapshot", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;

    REQUIRE(context.Contains(Data::ARPSnapshot));

    auto snapshot = context.Get<Data::ARPSnapshot>();

    REQUIRE(context.EverythingResult.Matches.size() == snapshot.size());

    // Destructively match
    for (const auto& match : context.EverythingResult.Matches)
    {
        bool found = false;
        for (auto itr = snapshot.begin(); itr != snapshot.end(); ++itr)
        {
            if (match.Package->GetProperty(PackageProperty::Id) == std::get<0>(*itr))
            {
                REQUIRE(match.Package->GetInstalledVersion()->GetProperty(PackageVersionProperty::Version) == std::get<1>(*itr));
                REQUIRE(match.Package->GetInstalledVersion()->GetProperty(PackageVersionProperty::Channel) == std::get<2>(*itr));

                snapshot.erase(itr);
                found = true;
                break;
            }
        }
        REQUIRE(found);
    }

    REQUIRE(snapshot.empty());
}

TEST_CASE("ARPChanges_NoChange_NoMatch", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context << ReportARPChanges;
    context.ExpectEvent(0, 0, 0);
}

TEST_CASE("ARPChanges_NoChange_SingleMatch", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddMatchResult("MatchId1", "MatchName1", "MatchPublisher1", "MatchVersion1");

    context << ReportARPChanges;
    context.ExpectEvent(0, 1, 0, context.MatchResult.Matches[0].Package.get());
}

TEST_CASE("ARPChanges_NoChange_MultiMatch", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddMatchResult("MatchId1", "MatchName1", "MatchPublisher1", "MatchVersion1");
    context.AddMatchResult("MatchId2", "MatchName2", "MatchPublisher2", "MatchVersion2");

    context << ReportARPChanges;
    context.ExpectEvent(0, 2, 0);
}

TEST_CASE("ARPChanges_SingleChange_NoMatch", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");

    context << ReportARPChanges;
    context.ExpectEvent(1, 0, 0, context.EverythingResult.Matches.back().Package.get());
}

TEST_CASE("ARPChanges_SingleChange_SingleMatch", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.AddMatchResult("MatchId1", "MatchName1", "MatchPublisher1", "MatchVersion1");

    context << ReportARPChanges;
    context.ExpectEvent(1, 1, 0, context.EverythingResult.Matches.back().Package.get());
}

TEST_CASE("ARPChanges_SingleChange_MultiMatch", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.AddMatchResult("MatchId1", "MatchName1", "MatchPublisher1", "MatchVersion1");
    context.MatchResult.Matches.emplace_back(context.EverythingResult.Matches.back());

    context << ReportARPChanges;
    context.ExpectEvent(1, 2, 1, context.EverythingResult.Matches.back().Package.get());
}

TEST_CASE("ARPChanges_MultiChange_NoMatch", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.AddEverythingResult("EverythingId2", "EverythingName2", "EverythingPublisher2", "EverythingVersion2");

    context << ReportARPChanges;
    context.ExpectEvent(2, 0, 0);
}

TEST_CASE("ARPChanges_MultiChange_SingleMatch_NoOverlap", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.AddEverythingResult("EverythingId2", "EverythingName2", "EverythingPublisher2", "EverythingVersion2");
    context.AddMatchResult("MatchId1", "MatchName1", "MatchPublisher1", "MatchVersion1");

    context << ReportARPChanges;
    context.ExpectEvent(2, 1, 0);
}

TEST_CASE("ARPChanges_MultiChange_SingleMatch_Overlap", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.AddEverythingResult("EverythingId2", "EverythingName2", "EverythingPublisher2", "EverythingVersion2");
    context.MatchResult.Matches.emplace_back(context.EverythingResult.Matches.back());

    context << ReportARPChanges;
    context.ExpectEvent(2, 1, 1, context.MatchResult.Matches.back().Package.get());
}

TEST_CASE("ARPChanges_MultiChange_MultiMatch_NoOverlap", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.AddEverythingResult("EverythingId2", "EverythingName2", "EverythingPublisher2", "EverythingVersion2");
    context.AddMatchResult("MatchId1", "MatchName1", "MatchPublisher1", "MatchVersion1");
    context.AddMatchResult("MatchId2", "MatchName2", "MatchPublisher2", "MatchVersion2");

    context << ReportARPChanges;
    context.ExpectEvent(2, 2, 0);
}

TEST_CASE("ARPChanges_MultiChange_MultiMatch_SingleOverlap", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.AddEverythingResult("EverythingId2", "EverythingName2", "EverythingPublisher2", "EverythingVersion2");
    context.AddMatchResult("MatchId1", "MatchName1", "MatchPublisher1", "MatchVersion1");
    context.MatchResult.Matches.emplace_back(context.EverythingResult.Matches.back());

    context << ReportARPChanges;
    context.ExpectEvent(2, 2, 1, context.MatchResult.Matches.back().Package.get());
}

TEST_CASE("ARPChanges_MultiChange_MultiMatch_MultiOverlap", "[ARPChanges][workflow]")
{
    TestContext context;

    context << SnapshotARPEntries;
    REQUIRE(context.Contains(Data::ARPSnapshot));

    context.AddEverythingResult("EverythingId1", "EverythingName1", "EverythingPublisher1", "EverythingVersion1");
    context.MatchResult.Matches.emplace_back(context.EverythingResult.Matches.back());
    context.AddEverythingResult("EverythingId2", "EverythingName2", "EverythingPublisher2", "EverythingVersion2");
    context.MatchResult.Matches.emplace_back(context.EverythingResult.Matches.back());

    context << ReportARPChanges;
    context.ExpectEvent(2, 2, 2);
}
