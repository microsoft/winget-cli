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
    std::chrono::milliseconds TotalTime;

    size_t TotalCases() const
    {
        return TrueMatches + TrueMismatches + FalseMatches + FalseMismatches;
    }

    auto AverageMatchingTime() const
    {
        return TotalTime / TotalCases();
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
    return ARPEntry{ TestPackage::Make(arpManifest, TestPackage::MetadataMap{}), false };
}

void ReportMatch(std::string_view label, std::string_view appName, std::string_view appPublisher, std::string_view arpName, std::string_view arpPublisher)
{
    WARN(label << '\n' <<
        "\tApp name      = " << appName << '\n' <<
        "\tApp publisher = " << appPublisher << '\n' <<
        "\tARP name      = " << arpName << '\n' <<
        "\tARP publisher = " << arpPublisher);
}

ResultSummary EvaluateDataSetWithHeuristic(const DataSet& dataSet, IARPMatchConfidenceAlgorithm& correlationAlgorithm, bool reportErrors = false)
{
    ResultSummary result{};
    auto startTime = std::chrono::steady_clock::now();

    // Each entry under test will be pushed at the end of this
    // and removed at the end.
    auto arpEntries = dataSet.ARPNoise;

    for (const auto& testCase : dataSet.TestCases)
    {
        arpEntries.push_back(GetARPEntryFromTestCase(testCase));
        auto match = FindARPEntryForNewlyInstalledPackageWithHeuristics(GetManifestFromTestCase(testCase), arpEntries, correlationAlgorithm);
        arpEntries.pop_back();

        if (match)
        {
            auto matchName = match->GetProperty(PackageVersionProperty::Name);
            auto matchPublisher = match->GetProperty(PackageVersionProperty::Publisher);

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

    auto endTime = std::chrono::steady_clock::now();
    result.TotalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

    return result;
}

void ReportResults(ResultSummary results)
{
    // This uses WARN to report as that is always shown regardless of the test result.
    // We may want to re-consider reporting in some other way
    WARN("Total cases:       " << results.TotalCases() << '\n' <<
         "True matches:      " << results.TrueMatches << '\n' <<
         "False matches:     " << results.FalseMatches << '\n' <<
         "True mismatches:   " << results.TrueMismatches << '\n' <<
         "False mismatches:  " << results.FalseMismatches << '\n' <<
         "Total matching time:   " << results.TotalTime.count() << "ms\n" <<
         "Average matching time: " << results.AverageMatchingTime().count() << "ms");
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
    // The format of the file is one case per line, each with pipe (|) separated values.
    // Each row contains: AppId, AppName, AppPublisher, ARPDisplayName, ARPDisplayVersion, ARPPublisherName, ARPProductCode
    // TODO: Add more test cases; particularly for non-matches
    std::ifstream testDataStream(TestCommon::TestDataFile("InputARPData.txt").GetPath());
    REQUIRE(testDataStream);

    std::vector<TestCase> testCases;

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

DataSet GetDataSet_NoNoise()
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

DataSet GetDataSet_WithNoise()
{
    DataSet dataSet;
    auto baseTestCases = LoadTestData();

    std::transform(baseTestCases.begin(), baseTestCases.end(), std::back_inserter(dataSet.ARPNoise), GetARPEntryFromTestCase);
    dataSet.TestCases = std::move(baseTestCases);

    // Arbitrary values. We should refine them as the algorithm gets better.
    dataSet.RequiredTrueMatchRatio = 0.5;
    dataSet.RequiredFalseMatchRatio = 0.1;
    dataSet.RequiredTrueMismatchRatio = 0; // There are no expected mismatches in this data set
    dataSet.RequiredFalseMismatchRatio = 0.5;

    return dataSet;
}

// Hide this test as it takes too long to run.
// It is useful for comparing multiple algorithms, but for
// regular testing we need only check that the chosen algorithm
// performs well.
TEMPLATE_TEST_CASE("Correlation_MeasureAlgorithmPerformance", "[correlation][.]",
    EmptyMatchConfidenceAlgorithm,
    EditDistanceMatchConfidenceAlgorithm)
{
    // Each section loads a different data set,
    // and then they are all handled the same
    DataSet dataSet;
    SECTION("No ARP noise")
    {
        dataSet = GetDataSet_NoNoise();
    }
    SECTION("With ARP noise")
    {
        dataSet = GetDataSet_WithNoise();
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
    SECTION("No ARP noise")
    {
        dataSet = GetDataSet_NoNoise();
    }
    SECTION("With ARP noise")
    {
        dataSet = GetDataSet_WithNoise();
    }

    // Use only the measure we ultimately pick
    auto& algorithm = IARPMatchConfidenceAlgorithm::Instance();
    auto results = EvaluateDataSetWithHeuristic(dataSet, algorithm, /* reportErrors */ true);
    ReportAndEvaluateResults(results, dataSet);
}
