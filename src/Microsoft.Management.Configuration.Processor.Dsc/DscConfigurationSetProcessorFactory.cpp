// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationSetProcessorFactory.h"
#include "DscConfigurationSetProcessor.h"

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;
    using namespace winrt::Windows::Foundation;

    IConfigurationSetProcessor DscConfigurationSetProcessorFactory::CreateSetProcessor(const ConfigurationSet& /*configurationSet*/)
    {
        return winrt::make<DscConfigurationSetProcessor>();
    }

    winrt::event_token DscConfigurationSetProcessorFactory::Diagnostics(const EventHandler<IDiagnosticInformation>& handler)
    {
        return m_diagnostics.add(handler);
    }

    void DscConfigurationSetProcessorFactory::Diagnostics(const winrt::event_token& token) noexcept
    {
        m_diagnostics.remove(token);
    }

    DiagnosticLevel DscConfigurationSetProcessorFactory::MinimumLevel()
    {
        return m_minimulLevel;
    }

    void DscConfigurationSetProcessorFactory::MinimumLevel(winrt::Microsoft::Management::Configuration::DiagnosticLevel value)
    {
        m_minimulLevel = value;
    }
}
