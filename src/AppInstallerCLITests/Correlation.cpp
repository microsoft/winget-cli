// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

#include <winget/ARPCorrelation.h>
#include <winget/Manifest.h>
#include <winget/RepositorySearch.h>

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Repository::Correlation;

// Data for defining a test case
struct TestCase
{
    // Actual app data
    std::string AppName;
    std::string AppPublisher;

    // Data in ARP
    std::string ArpName;
    std::string ArpPublisher;

    bool IsMatch;
};

struct ResultSummary
{
    unsigned TrueMatches;
    unsigned TrueMismatches;
    unsigned FalseMatches;
    unsigned FalseMismatches;

    unsigned TotalCases() const
    {
        return TrueMatches + TrueMismatches + FalseMatches + FalseMismatches;
    }
};

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
    std::string appId;
    std::string appName;
    std::string appPublisher;
    std::string arpDisplayName;
    std::string arpDisplayVersion;
    std::string arpPublisherName;
    std::string arpProductCode;
    while (std::getline(testDataStream, appId, '\t') &&
        std::getline(testDataStream, appName, '\t') &&
        std::getline(testDataStream, appPublisher, '\t') &&
        std::getline(testDataStream, arpDisplayName, '\t') &&
        std::getline(testDataStream, arpDisplayName, '\t') &&
        std::getline(testDataStream, arpPublisherName, '\t') &&
        std::getline(testDataStream, arpProductCode, '\t'))
    {
        TestCase testCase;
        std::swap(testCase.AppName, appName);
        std::swap(testCase.AppPublisher, appPublisher);
        std::swap(testCase.ArpName, arpDisplayName);
        std::swap(testCase.ArpPublisher, arpPublisherName);
        testCase.IsMatch = true;
    }

    return testCases;
}

ResultSummary EvaluateCorrelationMeasure(const ARPCorrelationMeasure& measure, const std::vector<TestCase>& cases)
{
    ResultSummary result{};
    for (const auto& testCase : cases)
    {
        // TODO: initialize with test data
        Manifest manifest;
        std::vector<ARPEntry> arpEntries;
        auto match = measure.GetBestMatchForManifest(manifest, arpEntries);

        if (match)
        {
            // TODO: Improve match check
            if (match->GetProperty(PackageVersionProperty::Name) == testCase.ArpName)
            {
                ++result.TrueMatches;
            }
            else
            {
                ++result.FalseMatches;
            }
        }
        else
        {
            if (testCase.IsMatch)
            {
                ++result.FalseMismatches;
            }
            else
            {
                ++result.TrueMismatches;
            }
        }
    }

    return result;
}

TEMPLATE_TEST_CASE("MeasureAlgorithmPerformance", "[correlation]",
    NoCorrelation,
    NormalizedNameAndPublisherCorrelation)
{
    TestType measure;
    std::vector<TestCase> testCases = LoadTestData();

    auto resultSummary = EvaluateCorrelationMeasure(measure, testCases);
    CAPTURE(resultSummary.TrueMatches, resultSummary.TrueMismatches, resultSummary.FalseMatches, resultSummary.FalseMismatches);
    // TODO: Log
    // TODO: Check against minimum expected
}