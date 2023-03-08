// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Documentation.g.h"
#include <winget/Manifest.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct Documentation : DocumentationT<Documentation>
    {
        Documentation() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Manifest::Documentation documentation);
#endif

        hstring DocumentLabel();
        hstring DocumentUrl();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Manifest::Documentation m_documentation{};
#endif
    };
}
