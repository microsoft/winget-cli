// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestConfigurationSetResult.h"
#include "TestConfigurationSetResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    TestConfigurationSetResult::TestConfigurationSetResult() : m_unitResults(multi_threaded_vector<TestConfigurationUnitResult>())
    {
    }

    void TestConfigurationSetResult::AppendUnitResult(const TestConfigurationUnitResult& unitResult)
    {
        m_unitResults.Append(unitResult);

        // Also aggregate the result of this incoming test into the overall result
        m_testResult = FoldInTestResult(m_testResult, unitResult.TestResult());
    }

    ConfigurationTestResult TestConfigurationSetResult::FoldInTestResult(ConfigurationTestResult current, ConfigurationTestResult incoming)
    {
        switch (current)
        {
        case ConfigurationTestResult::Unknown:
        case ConfigurationTestResult::NotRun:
            // In these "default" cases, just take the unit result
            return incoming;
            break;
        case ConfigurationTestResult::Positive:
            if (incoming == ConfigurationTestResult::Negative || incoming == ConfigurationTestResult::Failed)
            {
                return incoming;
            }
            break;
        case ConfigurationTestResult::Negative:
            if (incoming == ConfigurationTestResult::Failed)
            {
                return incoming;
            }
            break;
        case ConfigurationTestResult::Failed:
            // If a unit failed, the set failed
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return current;
    }

    Windows::Foundation::Collections::IVectorView<TestConfigurationUnitResult> TestConfigurationSetResult::UnitResults() const
    {
        return m_unitResults.GetView();
    }

    ConfigurationTestResult TestConfigurationSetResult::TestResult() const
    {
        return m_testResult;
    }

    void TestConfigurationSetResult::TestResult(ConfigurationTestResult value)
    {
        m_testResult = value;
    }
}
