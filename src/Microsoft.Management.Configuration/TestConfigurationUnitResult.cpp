// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestConfigurationUnitResult.h"
#include "TestConfigurationUnitResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void TestConfigurationUnitResult::Initialize(const ITestSettingsResult& result)
    {
        m_unit = result.Unit();
        THROW_HR_IF(E_POINTER, !m_unit);
        m_testResult = result.TestResult();
        m_resultInformation = result.ResultInformation();
        THROW_HR_IF(E_POINTER, !m_resultInformation);
    }

    void TestConfigurationUnitResult::Unit(const ConfigurationUnit& unit)
    {
        m_unit = unit;
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
