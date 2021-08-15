// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConnectResult.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ConnectResult : ConnectResultT<ConnectResult>
    {
        ConnectResult() = default;

        winrt::Microsoft::Management::Deployment::ConnectResultStatus Status();
        winrt::Microsoft::Management::Deployment::PackageCatalog PackageCatalog();
    };
}
