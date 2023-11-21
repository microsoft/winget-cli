// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestSettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    TestSettingsResult::TestSettingsResult(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit)
        : m_unit(unit) {}

    ConfigurationUnit TestSettingsResult::Unit()
    {
        return m_unit;
    }

    winrt::Microsoft::Management::Configuration::ConfigurationTestResult TestSettingsResult::TestResult()
    {
        return m_testResult;
    }

    void TestSettingsResult::TestResult(winrt::Microsoft::Management::Configuration::ConfigurationTestResult testResult)
    {
        m_testResult = testResult;
    }

    winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation TestSettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }

    void TestSettingsResult::ResultInformation(IConfigurationUnitResultInformation resultInformation)
    {
        m_resultInformation = resultInformation;
    }
}
