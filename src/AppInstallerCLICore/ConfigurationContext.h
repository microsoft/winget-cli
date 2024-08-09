// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <memory>
#include <vector>

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
        using ConfigurationSet = winrt::Microsoft::Management::Configuration::ConfigurationSet;

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

        ConfigurationSet& Set();
        const ConfigurationSet& Set() const;
        void Set(const ConfigurationSet& value);
        void Set(ConfigurationSet&& value);

        std::vector<ConfigurationSet>& History();
        const std::vector<ConfigurationSet>& History() const;
        void History(const winrt::Windows::Foundation::Collections::IVector<ConfigurationSet>& value);

    private:
        std::unique_ptr<details::ConfigurationContextData> m_data;
    };
}
