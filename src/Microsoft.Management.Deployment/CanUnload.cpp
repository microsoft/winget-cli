// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    static bool s_canUnload = true;

    void SetCanUnload(bool value)
    {
        s_canUnload = value;
    }

    bool GetCanUnload()
    {
        return s_canUnload;
    }
}
