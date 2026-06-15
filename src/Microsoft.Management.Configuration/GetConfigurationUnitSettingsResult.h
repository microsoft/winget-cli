// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetConfigurationUnitSettingsResult.g.h"
#include <winrt/Windows.Foundation.Collections.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetConfigurationUnitSettingsResult : GetConfigurationUnitSettingsResultT<GetConfigurationUnitSettingsResult>
    {
        GetConfigurationUnitSettingsResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void ResultInformation(const IConfigurationUnitResultInformation& resultInformation);
        void Settings(Windows::Foundation::Collections::ValueSet&& value);
#endif

        IConfigurationUnitResultInformation ResultInformation() const;
        Windows::Foundation::Collections::ValueSet Settings();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        IConfigurationUnitResultInformation m_resultInformation;
        Windows::Foundation::Collections::ValueSet m_settings;
#endif
    };
}
