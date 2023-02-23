// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestSettingsResult.h"
#include "TestSettingsResult.g.cpp"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    TestSettingsResult::TestSettingsResult() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    ConfigurationTestResult TestSettingsResult::TestResult()
    {
        return m_testResult;
    }

    void TestSettingsResult::TestResult(ConfigurationTestResult const& value)
    {
        m_testResult = value;
    }

    Configuration::ConfigurationUnitResultInformation TestSettingsResult::ResultInformation()
    {
        return m_resultInformation;
    }
}
