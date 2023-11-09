// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Unknwn.h>
#include <winrt/Windows.Foundation.h>

namespace AppInstaller::WinRT
{
    // Flag values for SetExperimentalState.
    enum class ConfigurationStaticsInternalsStateFlags : UINT32
    {
        None = 0,
        Configuration03 = 0x1,
        All = Configuration03
    };

    DEFINE_ENUM_FLAG_OPERATORS(ConfigurationStaticsInternalsStateFlags);

    MIDL_INTERFACE("C3886148-148A-4A3D-8018-9CDACDFC0B8D")
    IConfigurationStaticsInternals : public IUnknown
    {
    public:
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetExperimentalState(
            UINT32 state) = 0;
    };
}
