// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ApplyConfigurationSetResult.h"
#include "ApplyConfigurationSetResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ApplyConfigurationSetResult::Initialize(std::vector<ApplyConfigurationUnitResult>&& unitResults, hresult resultCode)
    {
        m_unitResults = single_threaded_vector(std::move(unitResults));
        m_resultCode = resultCode;
    }

    Windows::Foundation::Collections::IVectorView<ApplyConfigurationUnitResult> ApplyConfigurationSetResult::UnitResults()
    {
        return m_unitResults.GetView();
    }

    hresult ApplyConfigurationSetResult::ResultCode()
    {
        return m_resultCode;
    }
}
