// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <ConfigurationUnit.h>
#include <ConfigurationSet.h>
#include <ConfigurationSetUtilities.h>
#include <winget/Yaml.h>
#include <winrt/Windows.Storage.Streams.h>
#include <memory>
#include <string_view>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Interface for parsing a configuration set stream.
    struct ConfigurationSetParser
    {
        // Create a parser from the given bytes (the encoding is detected).
        static std::unique_ptr<ConfigurationSetParser> Create(std::string_view input);

        // Determines if the given value is a recognized schema version.
        // This will only return true for a version that we fully recognize.
        static bool IsRecognizedSchemaVersion(hstring value);

        // Determines if the given value is a recognized schema URI.
        // This will only return true for a URI that we fully recognize.
        static bool IsRecognizedSchemaUri(const Windows::Foundation::Uri& value);

        // Gets the schema URI associated with the given version, or null if there is not one.
        static Windows::Foundation::Uri GetSchemaUriForVersion(hstring value);

        // Gets the schema version associated with the given URI, or null if there is not one.
        static hstring GetSchemaVersionForUri(Windows::Foundation::Uri value);

        // Gets the schema version associated with the given URI, or null if there is not one.
        static std::string GetSchemaVersionForUri(std::string_view value);

        // Gets the latest schema version.
        static hstring LatestVersion();

        virtual ~ConfigurationSetParser() noexcept = default;

        ConfigurationSetParser(const ConfigurationSetParser&) = delete;
        ConfigurationSetParser& operator=(const ConfigurationSetParser&) = delete;
        ConfigurationSetParser(ConfigurationSetParser&&) = default;
        ConfigurationSetParser& operator=(ConfigurationSetParser&&) = default;

        // Parse the full document.
        virtual void Parse() = 0;

        // Retrieves the schema version of the parser.
        virtual hstring GetSchemaVersion() = 0;

        using ConfigurationSetPtr = decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>());

        // Retrieve the configuration set from the parser.
        ConfigurationSetPtr GetConfigurationSet() const { return m_configurationSet; }

        // The latest result code from the parser.
        hresult Result() const { return m_result; }

        // The field related to the result code.
        hstring Field() const { return m_field; }

        // The value of the field.
        hstring Value() const { return m_value; }

        // The line related to the result code.
        uint32_t Line() const { return m_line; }

        // The column related to the result code.
        uint32_t Column() const { return m_column; }

    protected:
        ConfigurationSetParser() = default;

        // Set the error state
        void SetError(hresult result, std::string_view field = {}, std::string_view value = {}, uint32_t line = 0, uint32_t column = 0);
        void SetError(hresult result, std::string_view field, const AppInstaller::YAML::Mark& mark, std::string_view value = {});

        ConfigurationSetPtr m_configurationSet;
        hresult m_result;
        hstring m_field;
        hstring m_value;
        uint32_t m_line = 0;
        uint32_t m_column = 0;

        // Gets the given `field` from the `parent` node, checking against the requirement and type.
        const AppInstaller::YAML::Node& GetAndEnsureField(const AppInstaller::YAML::Node& parent, ConfigurationField field, bool required, std::optional<AppInstaller::YAML::Node::Type> type);

        // Errors if the given `field` is present.
        void EnsureFieldAbsent(const AppInstaller::YAML::Node& parent, ConfigurationField field);

        // Parse the ValueSet named `field` from the given `node`.
        void ParseValueSet(const AppInstaller::YAML::Node& node, ConfigurationField field, bool required, const Windows::Foundation::Collections::ValueSet& valueSet);

        // Parse the mapping named `field` from the given `node`.
        void ParseMapping(const AppInstaller::YAML::Node& node, ConfigurationField field, bool required, AppInstaller::YAML::Node::Type elementType, std::function<void(std::string, const AppInstaller::YAML::Node&)> operation);

        // Parse the sequence named `field` from the given `node`.
        void ParseSequence(const AppInstaller::YAML::Node& node, ConfigurationField field, bool required, std::optional<AppInstaller::YAML::Node::Type> elementType, std::function<void(const AppInstaller::YAML::Node&)> operation);

        // Gets the string value in `field` from the given `node`, setting this value on `unit` using the `propertyFunction`.
        void GetStringValueForUnit(const AppInstaller::YAML::Node& node, ConfigurationField field, bool required, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(const hstring& value));

        // Gets the string array in `field` from the given `node`, setting this value on `unit` using the `propertyFunction`.
        void GetStringArrayForUnit(const AppInstaller::YAML::Node& node, ConfigurationField field, bool required, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(std::vector<hstring>&& value));

        // Validates the unit's Type property for correctness and consistency with the metadata. Should be called after parsing the Metadata value.
        void ValidateType(ConfigurationUnit* unit, const AppInstaller::YAML::Node& unitNode, ConfigurationField typeField, bool moveModuleNameToMetadata, bool moduleNameRequiredInType);

        // Parses an object from the given node, attempting to treat it as the requested type if possible.
        void ParseObject(const AppInstaller::YAML::Node& node, ConfigurationField fieldForErrors, Windows::Foundation::PropertyType type, Windows::Foundation::IInspectable& result);

    private:
        // Support older schema parsing.
        static std::unique_ptr<ConfigurationSetParser> GetSchemaVersionFromOldFormat(AppInstaller::YAML::Node& document, std::string& schemaVersionString);
    };
}
