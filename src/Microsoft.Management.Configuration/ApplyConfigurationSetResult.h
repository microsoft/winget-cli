// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplyConfigurationSetResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplyConfigurationSetResult : ApplyConfigurationSetResultT<ApplyConfigurationSetResult>
    {
        using ApplyConfigurationUnitResult = Configuration::ApplyConfigurationUnitResult;

        ApplyConfigurationSetResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(std::vector<ApplyConfigurationUnitResult>&& unitResults, hresult resultCode);
#endif

        Windows::Foundation::Collections::IVectorView<ApplyConfigurationUnitResult> UnitResults();
        hresult ResultCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Windows::Foundation::Collections::IVector<ApplyConfigurationUnitResult> m_unitResults = nullptr;
        hresult m_resultCode;
#endif
    };
}
