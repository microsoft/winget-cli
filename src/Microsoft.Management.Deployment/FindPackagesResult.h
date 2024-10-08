// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "FindPackagesResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesResult : FindPackagesResultT<FindPackagesResult>
    {
        FindPackagesResult() = default;
        
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            winrt::Microsoft::Management::Deployment::FindPackagesResultStatus status,
            bool wasLimitExceeded, 
            Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::MatchResult> matches,
            winrt::hresult extendedErrorCode);
#endif

        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus Status();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::MatchResult> Matches();
        bool WasLimitExceeded();
        winrt::hresult ExtendedErrorCode();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus m_status = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::Ok;
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::MatchResult> m_matches{ 
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::MatchResult>() };
        bool m_wasLimitExceeded = false;
        winrt::hresult m_extendedErrorCode = S_OK;
#endif
    };
}
