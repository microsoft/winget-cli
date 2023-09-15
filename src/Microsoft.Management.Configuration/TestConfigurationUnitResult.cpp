// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestConfigurationUnitResult.h"
#include "TestConfigurationUnitResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void TestConfigurationUnitResult::Initialize(ConfigurationUnit unit, IConfigurationUnitResultInformation resultInformation)
    {
        m_unit = unit;
        m_resultInformation = resultInformation;
    }

    ConfigurationUnit TestConfigurationUnitResult::Unit()
    {
        return m_unit;
    }

    IConfigurationUnitResultInformation TestConfigurationUnitResult::ResultInformation()
    {
        return m_resultInformation;
    }

    void TestConfigurationUnitResult::ResultInformation(const IConfigurationUnitResultInformation& value)
    {
        m_resultInformation = value;
    }

    ConfigurationTestResult TestConfigurationUnitResult::TestResult()
    {
        return m_testResult;
    }

    void TestConfigurationUnitResult::TestResult(ConfigurationTestResult value)
    {
        m_testResult = value;
    }
}
