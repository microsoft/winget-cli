// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "FindPackagesResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesResult : FindPackagesResultT<FindPackagesResult>
    {
        FindPackagesResult() = default;

        winrt::Microsoft::Management::Deployment::FindPackagesResultStatus Status();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::MatchResult> Matches();
        bool WasLimitExceeded();
    };
}
