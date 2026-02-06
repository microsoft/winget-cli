// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace winrt::Microsoft::Management::Deployment::implementation
{
    // Sets whether the module can unload or not.
    void SetCanUnload(bool value);

    // Gets whether the module can unload or not.
    bool GetCanUnload();
}
