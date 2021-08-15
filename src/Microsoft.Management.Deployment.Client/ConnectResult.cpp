// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConnectResult.h"
#include "ConnectResult.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    winrt::Microsoft::Management::Deployment::ConnectResultStatus ConnectResult::Status()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageCatalog ConnectResult::PackageCatalog()
    {
        throw hresult_not_implemented();
    }
}
