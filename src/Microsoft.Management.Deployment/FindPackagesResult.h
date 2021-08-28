// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "FindPackagesResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesResult : FindPackagesResultT<FindPackagesResult>
    {
        FindPackagesResult() = default;
        void Initialize(
            winrt::Microsoft::Management::Deployment::FindPackagesResultStatus errorCode,
            bool wasLimitExceeded, 
            Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::MatchResult> matches);

        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus Status();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::MatchResult> Matches();
        bool WasLimitExceeded();

    private:
        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus m_status = winrt::Microsoft::Management::Deployment::FindPackagesResultStatus::Ok;
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::MatchResult> m_matches{ 
            winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::MatchResult>() };
        bool m_wasLimitExceeded = false;
    };
}
