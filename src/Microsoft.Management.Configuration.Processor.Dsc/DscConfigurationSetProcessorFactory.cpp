// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationSetProcessorFactory.h"
#include "DscConfigurationSetProcessorFactory.g.cpp"
#include "DscConfigurationSetProcessor.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;
    using namespace winrt::Windows::Foundation;

    IConfigurationSetProcessor DscConfigurationSetProcessorFactory::CreateSetProcessor(ConfigurationSet const& configurationSet)
    {
        return winrt::make<DscConfigurationSetProcessor>(configurationSet, get_weak());
    }

    winrt::event_token DscConfigurationSetProcessorFactory::Diagnostics(EventHandler<IDiagnosticInformation> const& handler)
    {
        return m_diagnostics.add(handler);
    }

    void DscConfigurationSetProcessorFactory::Diagnostics(winrt::event_token const& token) noexcept
    {
        m_diagnostics.remove(token);
    }

    DiagnosticLevel DscConfigurationSetProcessorFactory::MinimumLevel()
    {
        return m_minimumLevel;
    }

    void DscConfigurationSetProcessorFactory::MinimumLevel(winrt::Microsoft::Management::Configuration::DiagnosticLevel const& minimumLevel)
    {
        m_minimumLevel = minimumLevel;
    }
}