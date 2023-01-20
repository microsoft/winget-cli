// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationProcessor.h"
#include "ConfigurationProcessor.g.cpp"
#include "ConfigurationSet.h"
#include "OpenConfigurationSetResult.h"
#include "ConfigurationSetParser.h"

#include <AppInstallerErrors.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ConfigurationProcessor::ConfigurationProcessor(const IConfigurationProcessorFactory& factory) : m_factory(factory)
    {
    }

    void ConfigurationProcessor::Initialize()
    {

    }

    event_token ConfigurationProcessor::Diagnostics(const Windows::Foundation::EventHandler<DiagnosticInformation>& handler)
    {
        return m_diagnostics.add(handler);
    }

    void ConfigurationProcessor::Diagnostics(const event_token& token) noexcept
    {
        m_diagnostics.remove(token);
    }

    event_token ConfigurationProcessor::ConfigurationChange(const Windows::Foundation::TypedEventHandler<ConfigurationSet, ConfigurationChangeData>& handler)
    {
        return m_configurationChange.add(handler);
    }

    void ConfigurationProcessor::ConfigurationChange(const event_token& token) noexcept
    {
        m_configurationChange.remove(token);
    }

    Windows::Foundation::Collections::IVector<Configuration::ConfigurationSet> ConfigurationProcessor::GetConfigurationHistory()
    {
        THROW_HR(E_NOTIMPL);
    }

    Configuration::OpenConfigurationSetResult ConfigurationProcessor::OpenConfigurationSet(Windows::Storage::Streams::IInputStream stream)
    {
        auto result = make_self<wil::details::module_count_wrapper<OpenConfigurationSetResult>>();

        try
        {
            std::unique_ptr<ConfigurationSetParser> parser = ConfigurationSetParser::Create(stream);
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field());
                return *result;
            }

            auto configurationSet = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();
            configurationSet->Initialize(parser->GetConfigurationUnits());
            if (FAILED(parser->Result()))
            {
                result->Initialize(parser->Result(), parser->Field());
                return *result;
            }

            result->Initialize(*configurationSet);
        }
        catch(const wil::ResultException& resultException)
        {
            result->Initialize(resultException.GetErrorCode());
        }
        catch (...)
        {
            result->Initialize(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE);
            LOG_CAUGHT_EXCEPTION();
        }

        return *result;
    }

    Windows::Foundation::IAsyncOperation<Windows::Foundation::Collections::IVectorView<ConfigurationConflict>> ConfigurationProcessor::CheckForConflictsAsync(
        const Windows::Foundation::Collections::IVectorView<ConfigurationSet>& configurationSets,
        bool includeConfigurationHistory)
    {
        UNREFERENCED_PARAMETER(configurationSets);
        UNREFERENCED_PARAMETER(includeConfigurationHistory);
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncAction ConfigurationProcessor::GetSetDetailsAsync(const ConfigurationSet& configurationSet, const ConfigurationUnitDetailLevel& detailLevel)
    {
        UNREFERENCED_PARAMETER(configurationSet);
        UNREFERENCED_PARAMETER(detailLevel);
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncAction ConfigurationProcessor::GetUnitDetailsAsync(const ConfigurationUnit& unit, const ConfigurationUnitDetailLevel& detailLevel)
    {
        UNREFERENCED_PARAMETER(unit);
        UNREFERENCED_PARAMETER(detailLevel);
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperation<ApplyConfigurationSetResult> ConfigurationProcessor::ApplyAsync(const ConfigurationSet& configurationSet, ApplyConfigurationSetFlags flags)
    {
        UNREFERENCED_PARAMETER(configurationSet);
        UNREFERENCED_PARAMETER(flags);
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperation<TestConfigurationSetResult> ConfigurationProcessor::TestAsync(const ConfigurationSet& configurationSet)
    {
        UNREFERENCED_PARAMETER(configurationSet);
        THROW_HR(E_NOTIMPL);
    }

    Windows::Foundation::IAsyncOperation<GetConfigurationUnitSettingsResult> ConfigurationProcessor::GetSettingsAsync(const ConfigurationUnit& unit)
    {
        UNREFERENCED_PARAMETER(unit);
        THROW_HR(E_NOTIMPL);
    }
}
