// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetConfigurationUnitSettingsResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetConfigurationUnitSettingsResult : GetConfigurationUnitSettingsResultT<GetConfigurationUnitSettingsResult>
    {
        GetConfigurationUnitSettingsResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize();
#endif

        ConfigurationUnitResultInformation ResultInformation();
        Windows::Foundation::Collections::ValueSet Settings();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnitResultInformation m_resultInformation = nullptr;
        Windows::Foundation::Collections::ValueSet m_settings = nullptr;
#endif
    };
}
