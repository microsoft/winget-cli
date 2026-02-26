// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationContext.h"
#include <winrt/Microsoft.Management.Configuration.h>

using namespace winrt::Microsoft::Management::Configuration;

namespace AppInstaller::CLI::Execution
{
    namespace details
    {
        struct ConfigurationContextData
        {
            ConfigurationProcessor Processor = nullptr;
            ConfigurationSet Set = nullptr;
            std::vector<ConfigurationSet> History;
        };
    }

    ConfigurationContext::ConfigurationContext() : m_data(std::make_unique<details::ConfigurationContextData>())
    {
    }

    ConfigurationContext::~ConfigurationContext() = default;

    ConfigurationContext::ConfigurationContext(ConfigurationContext&&) = default;
    ConfigurationContext& ConfigurationContext::operator=(ConfigurationContext&&) = default;

    ConfigurationProcessor& ConfigurationContext::Processor()
    {
        return m_data->Processor;
    }

    const ConfigurationProcessor& ConfigurationContext::Processor() const
    {
        return m_data->Processor;
    }

    void ConfigurationContext::Processor(const ConfigurationProcessor& value)
    {
        m_data->Processor = value;
    }

    void ConfigurationContext::Processor(ConfigurationProcessor&& value)
    {
        m_data->Processor = std::move(value);
    }

    ConfigurationSet& ConfigurationContext::Set()
    {
        return m_data->Set;
    }

    const ConfigurationSet& ConfigurationContext::Set() const
    {
        return m_data->Set;
    }

    void ConfigurationContext::Set(const ConfigurationSet& value)
    {
        m_data->Set = value;
    }

    void ConfigurationContext::Set(ConfigurationSet&& value)
    {
        m_data->Set = std::move(value);
    }

    std::vector<ConfigurationSet>& ConfigurationContext::History()
    {
        return m_data->History;
    }

    const std::vector<ConfigurationSet>& ConfigurationContext::History() const
    {
        return m_data->History;
    }

    void ConfigurationContext::History(const winrt::Windows::Foundation::Collections::IVector<ConfigurationSet>& value)
    {
        std::vector<ConfigurationSet> history{ value.Size() };
        value.GetMany(0, history);
        m_data->History = std::move(history);
    }
}
