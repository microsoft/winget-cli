// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Icon.g.h"
#include <winget/Manifest.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct Icon : IconT<Icon>
    {
        Icon() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Manifest::Icon icon);
#endif

        hstring Url();
        IconFileType FileType();
        IconResolution Resolution();
        IconTheme Theme();
        com_array<uint8_t> Sha256();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Manifest::Icon m_icon{};
#endif
    };
}
