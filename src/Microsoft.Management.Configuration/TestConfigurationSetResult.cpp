// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestConfigurationSetResult.h"
#include "TestConfigurationSetResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    TestConfigurationSetResult::TestConfigurationSetResult() : m_unitResults(single_threaded_vector<TestConfigurationUnitResult>())
    {
    }

    void TestConfigurationSetResult::AppendUnitResult(const TestConfigurationUnitResult& unitResult)
    {
        m_unitResults.Append(unitResult);
    }

    Windows::Foundation::Collections::IVectorView<TestConfigurationUnitResult> TestConfigurationSetResult::UnitResults()
    {
        return m_unitResults.GetView();
    }

    bool TestConfigurationSetResult::TestResult()
    {
        return m_testResult;
    }

    void TestConfigurationSetResult::TestResult(bool value)
    {
        m_testResult = value;
    }
}
