// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestSettingsResultInstance.h"
#include "TestSettingsResultInstance.g.cpp"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    TestSettingsResultInstance::TestSettingsResultInstance() :
        m_resultInformation(*make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    ConfigurationTestResult TestSettingsResultInstance::TestResult()
    {
        return m_testResult;
    }

    void TestSettingsResultInstance::TestResult(ConfigurationTestResult const& value)
    {
        m_testResult = value;
    }

    Configuration::IConfigurationUnitResultInformation TestSettingsResultInstance::ResultInformation()
    {
        return m_resultInformation;
    }
}
