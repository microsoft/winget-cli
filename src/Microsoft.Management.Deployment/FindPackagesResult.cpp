// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FindPackagesResult.h"
#include "FindPackagesResult.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void FindPackagesResult::Initialize(
        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus status,
        bool wasLimitExceeded, 
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::MatchResult> matches)
    {
        m_status = status;
        m_matches = matches;
        m_wasLimitExceeded = wasLimitExceeded;
    }
    winrt::Microsoft::Management::Deployment::FindPackagesResultStatus FindPackagesResult::Status()
    {
        return m_status;
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::MatchResult> FindPackagesResult::Matches()
    {
        return m_matches.GetView();
    }
    bool FindPackagesResult::WasLimitExceeded()
    {
        return m_wasLimitExceeded;
    }
}
