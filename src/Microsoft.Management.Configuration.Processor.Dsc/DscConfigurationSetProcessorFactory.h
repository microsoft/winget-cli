// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    struct DscConfigurationSetProcessorFactory : winrt::implements<DscConfigurationSetProcessorFactory, winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory>
    {
        winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor CreateSetProcessor(const winrt::Microsoft::Management::Configuration::ConfigurationSet& configurationSet);

        winrt::event_token Diagnostics(const winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::IDiagnosticInformation>& handler);
        void Diagnostics(const winrt::event_token& token) noexcept;

        winrt::Microsoft::Management::Configuration::DiagnosticLevel MinimumLevel();
        void MinimumLevel(winrt::Microsoft::Management::Configuration::DiagnosticLevel value);

    private:
        winrt::event<winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::IDiagnosticInformation>> m_diagnostics;
        winrt::Microsoft::Management::Configuration::DiagnosticLevel m_minimulLevel = winrt::Microsoft::Management::Configuration::DiagnosticLevel::Informational;
    };
}
