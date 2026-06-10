// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestConfigurationSetResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct TestConfigurationSetResult : TestConfigurationSetResultT<TestConfigurationSetResult>
    {
        TestConfigurationSetResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void AppendUnitResult(const TestConfigurationUnitResult& unitResult);
        void TestResult(ConfigurationTestResult value);

        static ConfigurationTestResult FoldInTestResult(ConfigurationTestResult current, ConfigurationTestResult incoming);
#endif

        Windows::Foundation::Collections::IVectorView<TestConfigurationUnitResult> UnitResults() const;
        ConfigurationTestResult TestResult() const;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Windows::Foundation::Collections::IVector<TestConfigurationUnitResult> m_unitResults = nullptr;
        ConfigurationTestResult m_testResult = ConfigurationTestResult::Unknown;
#endif
    };
}
