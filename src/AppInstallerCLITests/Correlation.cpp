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

TEMPLATE_TEST_CASE("MeasureAlgorithmPerformance", "[correlation]", NoCorrelation)
{
    TestType measure;
    std::vector<TestCase> testCases;

    auto resultSummary = EvaluateCorrelationMeasure(measure, testCases);

    // TODO: Log
    // TODO: Check against minimum expected
}