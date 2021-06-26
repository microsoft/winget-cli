// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "InstallResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;

        hstring CorrelationData();
        bool RebootRequired();
        winrt::Microsoft::Management::Deployment::InstallResultStatus Status();
        winrt::hresult ExtendedErrorCode();
    };
}
