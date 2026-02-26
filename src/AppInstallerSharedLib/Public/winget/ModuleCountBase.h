// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wrl\module.h>

namespace AppInstaller::WinRT
{
    // Implements module count interactions.
    struct ModuleCountBase
    {
        ModuleCountBase()
        {
            if (auto modulePtr = ::Microsoft::WRL::GetModuleBase())
            {
                modulePtr->IncrementObjectCount();
            }
        }

        ~ModuleCountBase()
        {
            if (auto modulePtr = ::Microsoft::WRL::GetModuleBase())
            {
                modulePtr->DecrementObjectCount();
            }
        }
    };
}
