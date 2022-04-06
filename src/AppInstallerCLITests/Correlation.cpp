// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

#include <winget/ARPCorrelation.h>
#include <winget/Manifest.h>
#include <winget/RepositorySearch.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Correlation;

using namespace TestCommon;

// Data for defining a test case
struct TestCase
{
    // Actual app data
    std::string AppName;
    std::string AppPublisher;

    // Data in ARP
    std::string ARPName;
    std::string ARPPublisher;

    bool IsMatch;
};

// Definition of a collection of test cases that we evaluate
// together to get a single aggregate result
struct DataSet
{
    // Details about the apps we are trying to correlate
    std::vector<TestCase> TestCases;

    // Additional ARP entries to use as "noise" for the correlation
    std::vector<ARPEntry> ARPNoise;

    // Thresholds for considering a run of an heuristic against
    // this data set "good".
    // Values are ratios to the total number of test cases
    double RequiredTrueMatchRatio;
    double RequiredTrueMismatchRatio;
    double RequiredFalseMatchRatio;
    double RequiredFalseMismatchRatio;
};

// Aggregate result of running an heuristic against a data set.
struct ResultSummary
{
    size_t TrueMatches;
    size_t TrueMismatches;
    size_t FalseMatches;
    size_t FalseMismatches;

    size_t TotalCases() const
    {
        return TrueMatches + TrueMismatches + FalseMatches + FalseMismatches;
    }
};

Manifest GetManifestFromTestCase(const TestCase& testCase)
{
    Manifest manifest;
    manifest.DefaultLocalization.Add<Localization::PackageName>(testCase.AppName);
    manifest.DefaultLocalization.Add<Localization::Publisher>(testCase.AppPublisher);
    manifest.Localizations.push_back(manifest.DefaultLocalization);
    return manifest;
}

ARPEntry GetARPEntryFromTestCase(const TestCase& testCase)
{
    Manifest arpManifest;
    arpManifest.DefaultLocalization.Add<Localization::PackageName>(testCase.ARPName);
    arpManifest.DefaultLocalization.Add<Localization::Publisher>(testCase.ARPPublisher);
    arpManifest.Localizations.push_back(arpManifest.DefaultLocalization);
    return ARPEntry{ TestPackageVersion::Make(arpManifest), false };
}

void ReportMatch(std::string_view label, std::string_view appName, std::string_view appPublisher, std::string_view arpName, std::string_view arpPublisher)
{
    WARN(label << '\n' <<
        "\tApp name      = " << appName << '\n' <<
        "\tApp publisher = " << appPublisher << '\n' <<
        "\tARP name      = " << arpName << '\n' <<
        "\tARP publisher = " << arpPublisher);
}

ResultSummary EvaluateDataSetWithHeuristic(const DataSet& dataSet, const ARPCorrelationAlgorithm& correlationAlgorithm, bool reportErrors = false)
{
    ResultSummary result{};

    // Each entry under test will be pushed at the end of this
    // and removed at the end.
    auto arpEntries = dataSet.ARPNoise;

    for (const auto& testCase : dataSet.TestCases)
    {
        arpEntries.push_back(GetARPEntryFromTestCase(testCase));
        auto match = correlationAlgorithm.GetBestMatchForManifest(GetManifestFromTestCase(testCase), arpEntries);
        arpEntries.pop_back();

        if (match)
        {
            auto matchManifest = match->Entry->GetManifest();
            auto matchName = matchManifest.DefaultLocalization.Get<Localization::PackageName>();
            auto matchPublisher = matchManifest.DefaultLocalization.Get <Localization::Publisher>();
            if (matchName == testCase.ARPName && matchPublisher == testCase.ARPPublisher)
            {
                ++result.TrueMatches;
            }
            else
            {
                ++result.FalseMatches;

                if (reportErrors)
                {
                    ReportMatch("False match", testCase.AppName, testCase.AppPublisher, matchName, matchPublisher);
                }
            }
        }
        else
        {
            if (testCase.IsMatch)
            {
                ++result.FalseMismatches;

                if (reportErrors)
                {
                    ReportMatch("False mismatch", testCase.AppName, testCase.AppPublisher, testCase.ARPName, testCase.ARPPublisher);
                }
            }
            else
            {
                ++result.TrueMismatches;
            }
        }
    }

    return result;
}

void ReportResults(ResultSummary results)
{
    // This uses WARN to report as that is always shown.
    // TODO: Consider reporting in some other way
    WARN("Total cases:       " << results.TotalCases() << '\n' <<
         "True matches:      " << results.TrueMatches << '\n' <<
         "False matches:     " << results.FalseMatches << '\n' <<
         "True mismatches:   " << results.TrueMismatches << '\n' <<
         "False mismatches:  " << results.FalseMismatches << '\n');
}

void ReportAndEvaluateResults(ResultSummary results, const DataSet& dataSet)
{
    ReportResults(results);

    // Required True ratio is a lower limit. The more results we get right, the better.
    // Required False ratio is an upper limit. The fewer results we get wrong, the better.
    REQUIRE(results.TrueMatches >= results.TotalCases() * dataSet.RequiredTrueMatchRatio);
    REQUIRE(results.TrueMismatches >= results.TotalCases() * dataSet.RequiredTrueMismatchRatio);
    REQUIRE(results.FalseMatches <= results.TotalCases() * dataSet.RequiredFalseMatchRatio);
    REQUIRE(results.FalseMismatches <= results.TotalCases()* dataSet.RequiredFalseMismatchRatio);
}

// TODO: Define multiple data sets
//   - Data set with many apps.
//   - Data set with popular apps. The match requirements should be higher
//   - Data set(s) in other languages.
//   - Data set where not everything has a match

std::vector<TestCase> LoadTestData()
{
    // Creates test cases from the test data file.
    // The format of the file is one case per line, each with tab separated values.
    // Each row contains: AppId, AppName, AppPublisher, ARPDisplayName, ARPDisplayVersion, ARPPublisherName, ARPProductCode
    // TODO: Cleanup data (e.g. bad encoding)
    //       Add more test cases; particularly for non-matches
    //       Consider using a different file format
    std::ifstream testDataStream(TestCommon::TestDataFile("InputARPData.txt").GetPath());
    REQUIRE(testDataStream);

    std::vector<TestCase> testCases;

    // TODO: There has to be a better way...
    std::string line;
    while (std::getline(testDataStream, line))
    {
        std::stringstream ss{ line };

        TestCase testCase;
        std::string appId;
        std::string arpDisplayVersion;
        std::string arpProductCode;
        std::getline(ss, appId, '|');
        std::getline(ss, testCase.AppName, '|');
        std::getline(ss, testCase.AppPublisher, '|');
        std::getline(ss, testCase.ARPName, '|');
        std::getline(ss, arpDisplayVersion, '|');
        std::getline(ss, testCase.ARPPublisher, '|');
        std::getline(ss, arpProductCode, '|');

        testCase.IsMatch = true;

        testCases.push_back(std::move(testCase));
    }

    return testCases;
}

DataSet GetDataSet_ManyAppsNoNoise()
{
    DataSet dataSet;
    dataSet.TestCases = LoadTestData();

    // Arbitrary values. We should refine them as the algorithm gets better.
    dataSet.RequiredTrueMatchRatio = 0.5;
    dataSet.RequiredFalseMatchRatio = 0.1;
    dataSet.RequiredTrueMismatchRatio = 0; // There are no expected mismatches in this data set
    dataSet.RequiredFalseMismatchRatio = 0.5;

    return dataSet;
}

DataSet GetDataSet_FewAppsMuchNoise()
{
    DataSet dataSet;
    auto baseTestCases = LoadTestData();

    std::transform(baseTestCases.begin(), baseTestCases.end(), std::back_inserter(dataSet.ARPNoise), GetARPEntryFromTestCase);

    // Take the first few apps from the test data
    for (size_t i = 0; i < 25; ++i)
    {
        dataSet.TestCases.push_back(baseTestCases[i]);
    }

    // Arbitrary values. We should refine them as the algorithm gets better.
    dataSet.RequiredTrueMatchRatio = 0.5;
    dataSet.RequiredFalseMatchRatio = 0.1;
    dataSet.RequiredTrueMismatchRatio = 0; // There are no expected mismatches in this data set
    dataSet.RequiredFalseMismatchRatio = 0.5;

    return dataSet;
}

// A correlation algorithm that considers only the matching with name+publisher.
// Used to evaluate the string matching.
template<typename T>
struct TestAlgorithmForStringMatching : public ARPCorrelationAlgorithm
{
    double GetMatchingScore(
        const Manifest&,
        const ManifestLocalization& manifestLocalization,
        const ARPEntry& arpEntry) const override
    {
        // Overall algorithm:
        // This considers only the matching between name/publisher.
        // It ignores versions and whether the ARP entry is new.
        const auto packageName = manifestLocalization.Get<Localization::PackageName>();
        const auto packagePublisher = manifestLocalization.Get<Localization::Publisher>();

        const auto arpNames = arpEntry.Entry->GetMultiProperty(PackageVersionMultiProperty::Name);
        const auto arpPublishers = arpEntry.Entry->GetMultiProperty(PackageVersionMultiProperty::Publisher);
        THROW_HR_IF(E_NOT_VALID_STATE, arpNames.size() != arpPublishers.size());

        T nameAndPublisherCorrelationMeasure;
        double bestMatch = 0;
        for (size_t i = 0; i < arpNames.size(); ++i)
        {
            bestMatch = std::max(bestMatch, nameAndPublisherCorrelationMeasure.GetMatchingScore(packageName, packagePublisher, arpNames[i], arpPublishers[i]));
        }

        return bestMatch;
    }
};


TEMPLATE_TEST_CASE("Correlation_MeasureAlgorithmPerformance", "[correlation]",
    TestAlgorithmForStringMatching<NormalizedNameAndPublisherCorrelationMeasure>,
    TestAlgorithmForStringMatching<EditDistanceNormalizedNameAndPublisherCorrelationMeasure>)
{
    // Each section loads a different data set,
    // and then they are all handled the same
    DataSet dataSet;
    SECTION("Many apps with no noise")
    {
        dataSet = GetDataSet_ManyAppsNoNoise();
    }
    SECTION("Few apps with much noise")
    {
        dataSet = GetDataSet_FewAppsMuchNoise();
    }

    TestType measure;
    auto results = EvaluateDataSetWithHeuristic(dataSet, measure);
    ReportResults(results);
}

TEST_CASE("Correlation_ChosenHeuristicIsGood", "[correlation]")
{
    // Each section loads a different data set,
    // and then they are all handled the same
    DataSet dataSet;
    SECTION("Many apps with no noise")
    {
        dataSet = GetDataSet_ManyAppsNoNoise();
    }
    SECTION("Few apps with much noise")
    {
        dataSet = GetDataSet_FewAppsMuchNoise();
    }

    // Use only the measure we ultimately pick
    const auto& measure = ARPCorrelationAlgorithm::Instance();
    auto results = EvaluateDataSetWithHeuristic(dataSet, measure, /* reportErrors */ true);
    ReportAndEvaluateResults(results, dataSet);
}
