// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "GetConfigurationSetDetailsResult.h"
#include "GetConfigurationSetDetailsResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    GetConfigurationSetDetailsResult::GetConfigurationSetDetailsResult() :
        m_unitResults(multi_threaded_vector<GetConfigurationUnitDetailsResult>())
    {};

    const Windows::Foundation::Collections::IVector<GetConfigurationUnitDetailsResult>& GetConfigurationSetDetailsResult::UnitResultsVector()
    {
        return m_unitResults;
    }

    Windows::Foundation::Collections::IVectorView<GetConfigurationUnitDetailsResult> GetConfigurationSetDetailsResult::UnitResults()
    {
        return m_unitResults.GetView();
    }
}
