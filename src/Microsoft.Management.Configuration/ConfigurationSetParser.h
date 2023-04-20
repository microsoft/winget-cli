// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <ConfigurationUnit.h>
#include <winrt/Windows.Storage.Streams.h>
#include <memory>
#include <string_view>
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

        // Determines if the given value is a recognized schema version.
        // This will only return true for a version that we fully recognize.
        static bool IsRecognizedSchemaVersion(hstring value);

        // Gets the latest schema version.
        static hstring LatestVersion();

        virtual ~ConfigurationSetParser() noexcept = default;

        ConfigurationSetParser(const ConfigurationSetParser&) = delete;
        ConfigurationSetParser& operator=(const ConfigurationSetParser&) = delete;
        ConfigurationSetParser(ConfigurationSetParser&&) = default;
        ConfigurationSetParser& operator=(ConfigurationSetParser&&) = default;

        // Retrieve the configuration units from the parser.
        virtual std::vector<Configuration::ConfigurationUnit> GetConfigurationUnits() = 0;

        // Retrieves the schema version of the parser.
        virtual hstring GetSchemaVersion() = 0;

        // The latest result code from the parser.
        hresult Result() const { return m_result; }

        // The field related to the result code.
        hstring Field() const { return m_field; }

    protected:
        ConfigurationSetParser() = default;

        // Set the error state
        void SetError(hresult result, std::string_view field = {});

        hresult m_result;
        hstring m_field;
    };
}
