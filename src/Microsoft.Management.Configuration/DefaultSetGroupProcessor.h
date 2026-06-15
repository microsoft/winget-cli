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

        void Initialize(const ConfigurationSet& set, const IConfigurationSetProcessor& setProcessor, ConfigThreadGlobals& threadGlobals, bool consistencyCheckOnly = false);

        IInspectable Group();

        Windows::Foundation::IAsyncOperation<ITestGroupSettingsResult> TestGroupSettingsAsync(Windows::Foundation::EventHandler<ITestSettingsResult> progressHandler);

        Windows::Foundation::IAsyncOperation<IApplyGroupSettingsResult> ApplyGroupSettingsAsync(Windows::Foundation::EventHandler<IApplyGroupMemberSettingsResult> progressHandler);

    private:
        void ThrowIf(bool cancellation);

        ConfigurationSet m_set = nullptr;
        IConfigurationSetProcessor m_setProcessor;
        ConfigThreadGlobals* m_threadGlobals = nullptr;
        bool m_consistencyCheckOnly = false;
    };
}
