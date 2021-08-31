// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <InstallResult.h>
#include "InstallResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring InstallResult::CorrelationData()
    {
        throw hresult_not_implemented();
    }
    bool InstallResult::RebootRequired()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::InstallResultStatus InstallResult::Status()
    {
        throw hresult_not_implemented();
    }
    winrt::hresult InstallResult::ExtendedErrorCode()
    {
        throw hresult_not_implemented();
    }
}
