// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "UninstallOptions.g.h"

namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct UninstallOptions : UninstallOptionsT<UninstallOptions, implementation::UninstallOptions>
    {
        auto ActivateInstance() const
        {
            return winrt::create_instance<winrt::Microsoft::Management::Deployment::UninstallOptions>(__uuidof(implementation::UninstallOptions), CLSCTX_ALL);
        }
    };
}
