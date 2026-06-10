// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestGroupSettingsResult.h"
#include "TestConfigurationSetResult.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    TestGroupSettingsResult::TestGroupSettingsResult() :
        m_resultInformation(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>()),
        m_unitResults(winrt::multi_threaded_vector<ITestSettingsResult>())
    {}

    void TestGroupSettingsResult::Group(const Windows::Foundation::IInspectable& value)
    {
        m_group = value;
    }

    void TestGroupSettingsResult::TestResult(ConfigurationTestResult value)
    {
        m_testResult = value;
    }

    TestGroupSettingsResult::ResultInformationPtr TestGroupSettingsResult::ResultInformationInternal()
    {
        return m_resultInformation;
    }

    void TestGroupSettingsResult::AppendUnitResult(const ITestSettingsResult& value)
    {
        m_unitResults.Append(value);

        // Also aggregate the result of this incoming test into the overall result
        m_testResult = TestConfigurationSetResult::FoldInTestResult(m_testResult, value.TestResult());
    }

    Windows::Foundation::IInspectable TestGroupSettingsResult::Group()
    {
        return m_group;
    }

    ConfigurationTestResult TestGroupSettingsResult::TestResult()
    {
        return m_testResult;
    }

    IConfigurationUnitResultInformation TestGroupSettingsResult::ResultInformation()
    {
        return *m_resultInformation;
    }

    Windows::Foundation::Collections::IVector<ITestSettingsResult> TestGroupSettingsResult::UnitResults()
    {
        return m_unitResults;
    }
}
