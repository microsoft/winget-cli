// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestConfigurationSetResult.h"
#include "TestConfigurationSetResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void TestConfigurationSetResult::Initialize()
    {

    }

    Windows::Foundation::Collections::IVectorView<TestConfigurationUnitResult> TestConfigurationSetResult::UnitResults()
    {
        return m_unitResults.GetView();
    }

    bool TestConfigurationSetResult::TestResult()
    {
        return m_testResult;
    }
}
