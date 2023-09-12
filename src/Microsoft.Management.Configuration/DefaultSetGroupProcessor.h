// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Implements the default group processing for configuration sets when the set processor doesn't handle it.
    struct DefaultSetGroupProcessor : winrt::implements<DefaultSetGroupProcessor, IConfigurationGroupProcessor>
    {
        DefaultSetGroupProcessor(const ConfigurationSet& set, const IConfigurationSetProcessor& setProcessor);

        IInspectable Group();

        Windows::Foundation::IAsyncOperationWithProgress<ITestGroupSettingsResult, ITestSettingsResult> TestGroupSettingsAsync();

        Windows::Foundation::IAsyncOperationWithProgress<IGetGroupSettingsResult, IGetSettingsResult> GetGroupSettingsAsync();

        Windows::Foundation::IAsyncOperationWithProgress<IApplyGroupSettingsResult, IApplySettingsResult> ApplyGroupSettingsAsync();

    private:
        ConfigurationSet m_set;
        IConfigurationSetProcessor m_setProcessor;
    };
}
