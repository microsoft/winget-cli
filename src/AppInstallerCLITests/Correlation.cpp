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
    std::string line;
    while (std::getline(testDataStream, line))
    {
        std::stringstream ss{ line };

        TestCase testCase;
        std::string appId;
        std::string arpDisplayVersion;
        std::string arpProductCode;
        std::getline(ss, appId, '\t');
        std::getline(ss, testCase.AppName, '\t');
        std::getline(ss, testCase.AppPublisher, '\t');
        std::getline(ss, testCase.ArpName, '\t');
        std::getline(ss, arpDisplayVersion, '\t');
        std::getline(ss, testCase.ArpPublisher, '\t');
        std::getline(ss, arpProductCode, '\t');

        testCase.IsMatch = true;

        testCases.push_back(std::move(testCase));
    }

    return testCases;
}

ResultSummary EvaluateCorrelationMeasure(const ARPCorrelationMeasure& measure, const std::vector<TestCase>& cases)
{
    std::vector<ARPEntry> allARPEntries;
    for (const auto& testCase : cases)
    {
        ARPEntry entry{ nullptr, true };
        entry.Name = testCase.ArpName;
        entry.Publisher = testCase.ArpPublisher;
        entry.IsNewOrUpdated = true;
        allARPEntries.push_back(entry);
    }

    ResultSummary result{};
    for (const auto& testCase : cases)
    {
        // TODO: initialize with test data
        Manifest manifest;
        manifest.DefaultLocalization.Add<Localization::PackageName>(testCase.AppName);
        manifest.DefaultLocalization.Add<Localization::Publisher>(testCase.AppPublisher);
        manifest.Localizations.push_back(manifest.DefaultLocalization);

        std::vector<ARPEntry> arpEntries;
        ARPEntry entry{ nullptr, true };
        entry.Name = testCase.ArpName;
        entry.Publisher = testCase.ArpPublisher;
        entry.IsNewOrUpdated = true;
        arpEntries.push_back(entry);
        // Add a couple of ARP entries as noise
        for (size_t i = 0; i < std::min((size_t)0, allARPEntries.size()); ++i)
        {
            arpEntries.push_back(allARPEntries[i]);
        }

        auto match = measure.GetBestMatchForManifest(manifest, arpEntries);

        if (match)
        {
            // TODO: Improve match check
            if (match->Name == testCase.ArpName && match->Publisher == testCase.ArpPublisher)
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
    WARN("True matches:\t" << resultSummary.TrueMatches);
    WARN("False matches:\t" << resultSummary.FalseMatches);
    WARN("True mismatches:\t" << resultSummary.TrueMismatches);
    WARN("False mismatches:\t" << resultSummary.FalseMismatches);
    WARN("Total cases:\t" << resultSummary.TotalCases());

    // TODO: Check against minimum expected
}