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

        ApplyConfigurationSetResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        const Windows::Foundation::Collections::IVector<ApplyConfigurationUnitResult>& UnitResultsVector();
        void ResultCode(hresult value);
#endif

        Windows::Foundation::Collections::IVectorView<ApplyConfigurationUnitResult> UnitResults() const;
        hresult ResultCode() const;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Windows::Foundation::Collections::IVector<ApplyConfigurationUnitResult> m_unitResults;
        hresult m_resultCode;
#endif
    };
}
