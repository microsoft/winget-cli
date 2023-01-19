// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <ConfigurationUnit.h>
#include <winrt/Windows.Storage.Streams.h>
#include <memory>
#include <vector>

using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Interface for parsing a configuration set stream.
    struct ConfigurationSetParser
    {
        static constexpr std::string_view NodeName_Properties = "properties"sv;
        static constexpr std::string_view NodeName_ConfigurationVersion = "configurationVersion"sv;

        // Create a parser from the given stream.
        static std::unique_ptr<ConfigurationSetParser> Create(const Windows::Storage::Streams::IInputStream& stream);

        // Create a parser from the given bytes (the encoding is detected).
        static std::unique_ptr<ConfigurationSetParser> Create(std::string_view input);

        virtual ~ConfigurationSetParser() noexcept = default;

        ConfigurationSetParser(const ConfigurationSetParser&) = delete;
        ConfigurationSetParser& operator=(const ConfigurationSetParser&) = delete;
        ConfigurationSetParser(ConfigurationSetParser&&) = default;
        ConfigurationSetParser& operator=(ConfigurationSetParser&&) = default;

        // Retrieve the configuration units from the parser.
        virtual std::vector<Configuration::ConfigurationUnit> GetConfigurationUnits() = 0;

    protected:
        ConfigurationSetParser() = default;
    };
}
