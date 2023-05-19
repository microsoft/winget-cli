// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestSettingsResultInstance.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct TestSettingsResultInstance : TestSettingsResultInstanceT<TestSettingsResultInstance>
    {
        TestSettingsResultInstance();

        ConfigurationTestResult TestResult();
        void TestResult(ConfigurationTestResult const& value);

        IConfigurationUnitResultInformation ResultInformation();

    private:
        ConfigurationTestResult m_testResult = ConfigurationTestResult::Unknown;
        IConfigurationUnitResultInformation m_resultInformation;
    };
}

namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct TestSettingsResultInstance : TestSettingsResultInstanceT<TestSettingsResultInstance, implementation::TestSettingsResultInstance>
    {
    };
}
