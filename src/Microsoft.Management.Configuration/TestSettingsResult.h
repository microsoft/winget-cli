// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestSettingsResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct TestSettingsResult : TestSettingsResultT<TestSettingsResult>
    {
        TestSettingsResult();

        ConfigurationTestResult TestResult();
        void TestResult(ConfigurationTestResult const& value);

        Configuration::ConfigurationUnitResultInformation ResultInformation();

    private:
        ConfigurationTestResult m_testResult = ConfigurationTestResult::Unknown;
        Configuration::ConfigurationUnitResultInformation m_resultInformation;
    };
}

namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct TestSettingsResult : TestSettingsResultT<TestSettingsResult, implementation::TestSettingsResult>
    {
    };
}
