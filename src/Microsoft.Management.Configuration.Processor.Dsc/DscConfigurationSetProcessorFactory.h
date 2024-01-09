// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "DscConfigurationSetProcessorFactory.g.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct DscConfigurationSetProcessorFactory : DscConfigurationSetProcessorFactoryT<DscConfigurationSetProcessorFactory>
    {
        DscConfigurationSetProcessorFactory() = default;

        winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor CreateSetProcessor(winrt::Microsoft::Management::Configuration::ConfigurationSet const& configurationSet);

        winrt::event_token Diagnostics(winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::IDiagnosticInformation> const& handler);
        void Diagnostics(winrt::event_token const& token) noexcept;

        winrt::Microsoft::Management::Configuration::DiagnosticLevel MinimumLevel();
        void MinimumLevel(winrt::Microsoft::Management::Configuration::DiagnosticLevel const& minimumLevel);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::event<winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::IDiagnosticInformation>> m_diagnostics;
        winrt::Microsoft::Management::Configuration::DiagnosticLevel m_minimumLevel = winrt::Microsoft::Management::Configuration::DiagnosticLevel::Informational;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::factory_implementation
{
    struct DscConfigurationSetProcessorFactory : DscConfigurationSetProcessorFactoryT<DscConfigurationSetProcessorFactory, implementation::DscConfigurationSetProcessorFactory>
    {
    };
}
#endif
