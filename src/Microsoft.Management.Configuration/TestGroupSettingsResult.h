// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct TestGroupSettingsResult : winrt::implements<TestGroupSettingsResult, ITestGroupSettingsResult>
    {
        TestGroupSettingsResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        using ResultInformationPtr = decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>());

        void Group(const Windows::Foundation::IInspectable& value);
        void TestResult(ConfigurationTestResult value);
        ResultInformationPtr ResultInformationInternal();
        void AppendUnitResult(const ITestSettingsResult& value);
#endif

        Windows::Foundation::IInspectable Group();
        ConfigurationTestResult TestResult();
        IConfigurationUnitResultInformation ResultInformation();
        Windows::Foundation::Collections::IVector<ITestSettingsResult> UnitResults();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Windows::Foundation::IInspectable m_group;
        ConfigurationTestResult m_testResult = ConfigurationTestResult::Unknown;
        ResultInformationPtr m_resultInformation;
        Windows::Foundation::Collections::IVector<ITestSettingsResult> m_unitResults;
#endif
    };
}
