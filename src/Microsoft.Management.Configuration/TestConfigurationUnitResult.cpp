// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestConfigurationUnitResult.h"
#include "TestConfigurationUnitResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void TestConfigurationUnitResult::Initialize(ConfigurationUnit unit, ConfigurationUnitResultInformation resultInformation)
    {
        m_unit = unit;
        m_resultInformation = resultInformation;
    }

    ConfigurationUnit TestConfigurationUnitResult::Unit()
    {
        return m_unit;
    }

    ConfigurationUnitResultInformation TestConfigurationUnitResult::ResultInformation()
    {
        return m_resultInformation;
    }

    bool TestConfigurationUnitResult::TestResult()
    {
        return m_testResult;
    }

    void TestConfigurationUnitResult::TestResult(bool value)
    {
        m_testResult = value;
    }
}
