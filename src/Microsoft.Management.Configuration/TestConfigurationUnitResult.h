// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestConfigurationUnitResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct TestConfigurationUnitResult : TestConfigurationUnitResultT<TestConfigurationUnitResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ConfigurationUnitResultInformation = Configuration::ConfigurationUnitResultInformation;

        TestConfigurationUnitResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(ConfigurationUnit unit, ConfigurationUnitResultInformation resultInformation);
        void TestResult(bool value);
#endif

        ConfigurationUnit Unit();
        ConfigurationUnitResultInformation ResultInformation();
        bool TestResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        ConfigurationUnitResultInformation m_resultInformation = nullptr;
        bool m_testResult;
#endif
    };
}
