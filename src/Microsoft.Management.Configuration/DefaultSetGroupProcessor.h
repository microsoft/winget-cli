// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include "ConfigThreadGlobals.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Implements the default group processing for configuration sets when the set processor doesn't handle it.
    struct DefaultSetGroupProcessor : winrt::implements<DefaultSetGroupProcessor, IConfigurationGroupProcessor>
    {
        using ConfigurationSet = Configuration::ConfigurationSet;

        DefaultSetGroupProcessor() = default;

        void Initialize(const ConfigurationSet& set, const IConfigurationSetProcessor& setProcessor, ConfigThreadGlobals& threadGlobals);

        IInspectable Group();

        Windows::Foundation::IAsyncOperationWithProgress<ITestGroupSettingsResult, ITestSettingsResult> TestGroupSettingsAsync();

        Windows::Foundation::IAsyncOperationWithProgress<IGetGroupSettingsResult, IGetSettingsResult> GetGroupSettingsAsync();

        Windows::Foundation::IAsyncOperationWithProgress<IApplyGroupSettingsResult, IApplySettingsResult> ApplyGroupSettingsAsync();

    private:
        void ThrowIf(bool cancellation);

        ConfigurationSet m_set = nullptr;
        IConfigurationSetProcessor m_setProcessor;
        ConfigThreadGlobals* m_threadGlobals = nullptr;
    };
}
