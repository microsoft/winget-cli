// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <memory>

namespace winrt::Microsoft::Management::Configuration
{
    struct ConfigurationProcessor;
    struct ConfigurationSet;
}

namespace AppInstaller::CLI::Execution
{
    namespace details
    {
        struct ConfigurationContextData;
    }

    struct ConfigurationContext
    {
        ConfigurationContext();
        ~ConfigurationContext();

        ConfigurationContext(ConfigurationContext&) = delete;
        ConfigurationContext& operator=(ConfigurationContext&) = delete;

        ConfigurationContext(ConfigurationContext&&);
        ConfigurationContext& operator=(ConfigurationContext&&);

        winrt::Microsoft::Management::Configuration::ConfigurationProcessor& Processor();
        const winrt::Microsoft::Management::Configuration::ConfigurationProcessor& Processor() const;
        void Processor(const winrt::Microsoft::Management::Configuration::ConfigurationProcessor& value);
        void Processor(winrt::Microsoft::Management::Configuration::ConfigurationProcessor&& value);

        winrt::Microsoft::Management::Configuration::ConfigurationSet& Set();
        const winrt::Microsoft::Management::Configuration::ConfigurationSet& Set() const;
        void Set(const winrt::Microsoft::Management::Configuration::ConfigurationSet& value);
        void Set(winrt::Microsoft::Management::Configuration::ConfigurationSet&& value);

    private:
        std::unique_ptr<details::ConfigurationContextData> m_data;
    };
}
