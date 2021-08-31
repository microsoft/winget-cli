// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <FindPackagesResult.h>
#include "FindPackagesResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::FindPackagesResultStatus FindPackagesResult::Status()
    {
        throw hresult_not_implemented();
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::MatchResult> FindPackagesResult::Matches()
    {
        throw hresult_not_implemented();
    }
    bool FindPackagesResult::WasLimitExceeded()
    {
        throw hresult_not_implemented();
    }
}
