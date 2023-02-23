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

        ConfigurationTestResult unitValue = unitResult.TestResult();

        // Also aggregate the result of this incoming test into the overall result
        switch (m_testResult)
        {
        case ConfigurationTestResult::Unknown:
        case ConfigurationTestResult::NotRun:
            // In these "default" cases, just take the unit result
            m_testResult = unitValue;
            break;
        case ConfigurationTestResult::Positive:
            if (unitValue == ConfigurationTestResult::Negative || unitValue == ConfigurationTestResult::Failed)
            {
                m_testResult = unitValue;
            }
            break;
        case ConfigurationTestResult::Negative:
            if (unitValue == ConfigurationTestResult::Failed)
            {
                m_testResult = unitValue;
            }
            break;
        case ConfigurationTestResult::Failed:
            // If a unit failed, the set failed
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    Windows::Foundation::Collections::IVectorView<TestConfigurationUnitResult> TestConfigurationSetResult::UnitResults()
    {
        return m_unitResults.GetView();
    }

    ConfigurationTestResult TestConfigurationSetResult::TestResult()
    {
        return m_testResult;
    }

    void TestConfigurationSetResult::TestResult(ConfigurationTestResult value)
    {
        m_testResult = value;
    }
}
