// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetConfigurationSetDetailsResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetConfigurationSetDetailsResult : GetConfigurationSetDetailsResultT<GetConfigurationSetDetailsResult>
    {
        using GetConfigurationUnitDetailsResult = Configuration::GetConfigurationUnitDetailsResult;

        GetConfigurationSetDetailsResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        const Windows::Foundation::Collections::IVector<GetConfigurationUnitDetailsResult>& UnitResultsVector();
#endif

        Windows::Foundation::Collections::IVectorView<GetConfigurationUnitDetailsResult> UnitResults();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Windows::Foundation::Collections::IVector<GetConfigurationUnitDetailsResult> m_unitResults;
#endif
    };
}
