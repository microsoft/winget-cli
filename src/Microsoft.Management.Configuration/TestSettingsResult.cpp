// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestSettingsResult.h"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void TestSettingsResult::Unit(const ConfigurationUnit& value)
    {
        m_unit = value;
    }

    void TestSettingsResult::TestResult(ConfigurationTestResult value)
    {
        m_testResult = value;
    }

    void TestSettingsResult::ResultInformation(const IConfigurationUnitResultInformation& value)
    {
        m_resultInformation = value;
    }

    ConfigurationUnit TestSettingsResult::Unit()
    {
        return m_unit;
    }

    ConfigurationTestResult TestSettingsResult::TestResult()
    {
        return m_testResult;
    }

    IConfigurationUnitResultInformation TestSettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }
}
