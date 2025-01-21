// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser.h"
#include "ParsingMacros.h"
#include "ArgumentValidation.h"

#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>

#include "ConfigurationSetUtilities.h"
#include "ConfigurationSetParserError.h"
#include "ConfigurationSetParser_0_1.h"
#include "ConfigurationSetParser_0_2.h"
#include "ConfigurationSetParser_0_3.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        struct SchemaVersionAndUri
        {
            std::string_view Version;
            std::wstring_view VersionWide;
            std::string_view Uri;
            std::wstring_view UriWide;
        };

#define SCHEMA_VERSION_MAP_ITEM(_version_,_uri_) _version_, TEXT(_version_), _uri_, TEXT(_uri_)

        // Please keep in sorted order with the highest version last.
        // Duplicate URIs are supported, but duplicate versions are not. The highest version for a URI will be the one mapped to, the lower versions will be aliases.
        SchemaVersionAndUri SchemaVersionAndUriMap[] =
        {
            { SCHEMA_VERSION_MAP_ITEM("0.1", "") },
            { SCHEMA_VERSION_MAP_ITEM("0.2", "") },
            { SCHEMA_VERSION_MAP_ITEM("0.3", "https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json") },
        };

        Windows::Foundation::IInspectable GetIInspectableFromNode(const Node& node);

        // Fills the ValueSet from the given node, which is assumed to be a map.
        void FillValueSetFromMap(const Node& mapNode, const Windows::Foundation::Collections::ValueSet& valueSet)
        {
            for (const auto& mapItem : mapNode.Mapping())
            {
                // Insert returns true if it replaces an existing key, and that indicates an invalid map.
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, valueSet.Insert(mapItem.first.as<std::wstring>(), GetIInspectableFromNode(mapItem.second)));
            }
        }

        // Returns the appropriate IPropertyValue for the given node, which is assumed to be a scalar.
        Windows::Foundation::IInspectable GetPropertyValueFromScalar(const Node& node)
        {
            ::winrt::Windows::Foundation::IInspectable result;

            switch (node.GetTagType())
            {
            case Node::TagType::Null:
                return Windows::Foundation::PropertyValue::CreateEmpty();
            case Node::TagType::Bool:
                return Windows::Foundation::PropertyValue::CreateBoolean(node.as<bool>());
            case Node::TagType::Str:
                return Windows::Foundation::PropertyValue::CreateString(node.as<std::wstring>());
            case Node::TagType::Int:
                return Windows::Foundation::PropertyValue::CreateInt64(node.as<int64_t>());
            case Node::TagType::Float:
                THROW_HR(E_NOTIMPL);
            case Node::TagType::Timestamp:
                THROW_HR(E_NOTIMPL);
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

        // Returns the appropriate IPropertyValue for the given node, which is assumed to be a scalar.
        Windows::Foundation::IInspectable GetPropertyValueFromSequence(const Node& sequenceNode)
        {
            Windows::Foundation::Collections::ValueSet result;
            size_t index = 0;

            for (const Node& sequenceItem : sequenceNode.Sequence())
            {
                std::wostringstream strstr;
                strstr << index++;
                result.Insert(strstr.str(), GetIInspectableFromNode(sequenceItem));
            }

            result.Insert(L"treatAsArray", Windows::Foundation::PropertyValue::CreateBoolean(true));
            return result;
        }

        // Returns the appropriate IInspectable for the given node.
        Windows::Foundation::IInspectable GetIInspectableFromNode(const Node& node)
        {
            ::winrt::Windows::Foundation::IInspectable result;

            switch (node.GetType())
            {
            case Node::Type::Invalid:
            case Node::Type::None:
                // Leave value as null
                break;
            case Node::Type::Scalar:
                result = GetPropertyValueFromScalar(node);
                break;
            case Node::Type::Sequence:
                result = GetPropertyValueFromSequence(node);
                break;
            case Node::Type::Mapping:
            {
                Windows::Foundation::Collections::ValueSet subset;
                FillValueSetFromMap(node, subset);
                result = std::move(subset);
            }
            break;
            default:
                THROW_HR(E_UNEXPECTED);
            }

            return result;
        }

        // Contains the qualified resource name information.
        struct QualifiedResourceName
        {
            QualifiedResourceName(hstring input)
            {
                std::wstring_view inputView = input;
                size_t pos = inputView.find('/');

                if (pos != std::wstring_view::npos)
                {
                    Module = inputView.substr(0, pos);
                    Resource = inputView.substr(pos + 1);
                }
                else
                {
                    Resource = input;
                }
            }

            hstring Module;
            hstring Resource;
        };
    }

    std::unique_ptr<ConfigurationSetParser> ConfigurationSetParser::Create(std::string_view input)
    {
        AICLI_LOG_LARGE_STRING(Config, Verbose, << "Parsing configuration set:", input);

        Node document;
        std::string documentError;
        Mark documentErrorMark;

        try
        {
            document = Load(input);
        }
        catch (const Exception& exc)
        {
            documentError = exc.what();
            documentErrorMark = exc.GetMark();
        }
        CATCH_LOG();

        if (!document.IsMap())
        {
            AICLI_LOG(Config, Error, << "Invalid YAML: " << documentError << " at [line " << documentErrorMark.line << ", col " << documentErrorMark.column << "]");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_YAML, documentError, documentErrorMark);
        }

        // The schema version for parsing the rest of the document
        std::string schemaUriString;
        std::string schemaVersionString;

        Node& schemaNode = document[GetConfigurationFieldName(ConfigurationField::Schema)];
        if (schemaNode.IsScalar())
        {
            schemaUriString = schemaNode.as<std::string>();
            schemaVersionString = GetSchemaVersionForUri(schemaUriString);
            AICLI_LOG(Config, Verbose, << "Configuration schema `" << schemaNode.as<std::string>() << "` mapped to version `" << schemaVersionString << "`.");
        }

        // If we recognize the schema, use that version.
        // If we didn't recognize it, try using the older format.
        if (schemaVersionString.empty())
        {
            std::unique_ptr<ConfigurationSetParser> oldFormatError = GetSchemaVersionFromOldFormat(document, schemaVersionString);

            // We have no schema version at all...
            if (oldFormatError)
            {
                // If the schema was provided and we didn't recognize it, make that the error.
                if (schemaNode.IsScalar())
                {
                    AICLI_LOG(Config, Error, << "Unknown configuration schema: " << schemaUriString);
                    return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, GetConfigurationFieldName(ConfigurationField::Schema), schemaUriString);
                }
                else
                {
                    // Otherwise, this is an older format file (or neither). The proper error came back from that function.
                    return oldFormatError;
                }
            }
        }

        // Create the parser based on the version selected
        auto result = CreateForSchemaVersion(std::move(schemaVersionString));
        result->SetDocument(std::move(document));
        return result;
    }

    std::unique_ptr<ConfigurationSetParser> ConfigurationSetParser::CreateForSchemaVersion(std::string input)
    {
        SemanticVersion schemaVersion(std::move(input));

        // TODO: Consider having the version/uri/type information all together in the future
        if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 1)
        {
            return std::make_unique<ConfigurationSetParser_0_1>();
        }
        else if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 2)
        {
            return std::make_unique<ConfigurationSetParser_0_2>();
        }
        else if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 3)
        {
            return std::make_unique<ConfigurationSetParser_0_3>();
        }

        AICLI_LOG(Config, Error, << "Unknown configuration version: " << schemaVersion.ToString());
        return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, GetConfigurationFieldName(ConfigurationField::ConfigurationVersion), schemaVersion.ToString());
    }

    bool ConfigurationSetParser::IsRecognizedSchemaVersion(hstring value) try
    {
        SemanticVersion schemaVersion(ConvertToUTF8(value));

        for (const auto& item : SchemaVersionAndUriMap)
        {
            if (schemaVersion == SemanticVersion{ std::string{ item.Version } })
            {
                return true;
            }
        }

        return false;
    }
    catch (...) { LOG_CAUGHT_EXCEPTION(); return false; }

    bool ConfigurationSetParser::IsRecognizedSchemaUri(const Windows::Foundation::Uri& value)
    {
        return !GetSchemaVersionForUri(value).empty();
    }

    Windows::Foundation::Uri ConfigurationSetParser::GetSchemaUriForVersion(hstring value)
    {
        for (const auto& item : SchemaVersionAndUriMap)
        {
            if (value == item.VersionWide)
            {
                return item.Uri.empty() ? nullptr : Windows::Foundation::Uri{ item.UriWide };
            }
        }

        return nullptr;
    }

    hstring ConfigurationSetParser::GetSchemaVersionForUri(Windows::Foundation::Uri value)
    {
        // Do a reverse search in order to give the highest version back for a given URI.
        auto itr = std::rbegin(SchemaVersionAndUriMap);
        auto end = std::rend(SchemaVersionAndUriMap);
        for (; itr != end; ++itr)
        {
            const auto& item = *itr;
            if (!item.Uri.empty())
            {
                Windows::Foundation::Uri uri{ item.UriWide };
                if (value.Equals(uri))
                {
                    return hstring{ item.VersionWide };
                }
            }
        }

        return {};
    }

    std::string ConfigurationSetParser::GetSchemaVersionForUri(std::string_view value)
    {
        // Do a reverse search in order to give the highest version back for a given URI.
        auto itr = std::rbegin(SchemaVersionAndUriMap);
        auto end = std::rend(SchemaVersionAndUriMap);
        for (; itr != end; ++itr)
        {
            const auto& item = *itr;
            if (!item.Uri.empty())
            {
                if (item.Uri == value)
                {
                    return std::string{ item.Version };
                }
            }
        }

        return {};
    }

    std::pair<hstring, Windows::Foundation::Uri> ConfigurationSetParser::LatestVersion()
    {
        auto latest = std::rbegin(SchemaVersionAndUriMap);
        return { hstring{ latest->VersionWide }, Windows::Foundation::Uri{ latest->UriWide } };
    }

    Windows::Foundation::Collections::ValueSet ConfigurationSetParser::ParseValueSet(std::string_view input)
    {
        Windows::Foundation::Collections::ValueSet result;
        FillValueSetFromMap(Load(input), result);
        return result;
    }

    std::vector<hstring> ConfigurationSetParser::ParseStringArray(std::string_view input)
    {
        std::vector<hstring> result;
        ParseSequence(Load(input), "string_array", Node::Type::Scalar, [&](const AppInstaller::YAML::Node& item)
        {
            result.emplace_back(item.as<std::wstring>());
        });
        return result;
    }

    void ConfigurationSetParser::SetError(hresult result, std::string_view field, std::string_view value, uint32_t line, uint32_t column)
    {
        AICLI_LOG(Config, Error, << "ConfigurationSetParser error: " << AppInstaller::Logging::SetHRFormat << result << " for " << field << " with value `" << value << "` at [line " << line << ", col " << column << "]");
        m_result = result;
        m_field = ConvertToUTF16(field);
        m_value = ConvertToUTF16(value);
        m_line = line;
        m_column = column;
    }

    void ConfigurationSetParser::SetError(hresult result, std::string_view field, const Mark& mark, std::string_view value)
    {
        SetError(result, field, value, static_cast<uint32_t>(mark.line), static_cast<uint32_t>(mark.column));
    }

    const Node& ConfigurationSetParser::GetAndEnsureField(const Node& parent, ConfigurationField field, bool required, std::optional<Node::Type> type)
    {
        const Node& fieldNode = parent[GetConfigurationFieldName(field)];

        if (fieldNode)
        {
            if (type && fieldNode.GetType() != type.value())
            {
                SetError(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, GetConfigurationFieldName(field), fieldNode.Mark());
            }
        }
        else if (required)
        {
            SetError(WINGET_CONFIG_ERROR_MISSING_FIELD, GetConfigurationFieldName(field));
        }

        return fieldNode;
    }

    void ConfigurationSetParser::EnsureFieldAbsent(const Node& parent, ConfigurationField field)
    {
        const Node& fieldNode = parent[GetConfigurationFieldName(field)];

        if (fieldNode)
        {
            SetError(WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, GetConfigurationFieldName(field), fieldNode.Mark(), fieldNode.as<std::string>());
        }
    }

    void ConfigurationSetParser::ParseValueSet(const Node& node, ConfigurationField field, bool required, const Windows::Foundation::Collections::ValueSet& valueSet)
    {
        const Node& mapNode = CHECK_ERROR(GetAndEnsureField(node, field, required, Node::Type::Mapping));

        if (mapNode)
        {
            FillValueSetFromMap(mapNode, valueSet);
        }
    }

    void ConfigurationSetParser::ParseMapping(const AppInstaller::YAML::Node& node, ConfigurationField field, bool required, AppInstaller::YAML::Node::Type elementType, std::function<void(std::string, const AppInstaller::YAML::Node&)> operation)
    {
        const Node& mapNode = CHECK_ERROR(GetAndEnsureField(node, field, required, Node::Type::Mapping));
        if (!mapNode)
        {
            return;
        }

        std::ostringstream strstr;
        strstr << GetConfigurationFieldName(field);
        size_t index = 0;

        for (const auto& mapItem : mapNode.Mapping())
        {
            std::string name = mapItem.first.as<std::string>();
            if (name.empty())
            {
                strstr << '[' << index << ']';
                FIELD_VALUE_ERROR(strstr.str(), name, mapItem.first.Mark());
            }

            if (mapItem.second.GetType() != elementType)
            {
                strstr << '[' << index << ']';
                FIELD_TYPE_ERROR(strstr.str(), mapItem.second.Mark());
            }
            index++;

            CHECK_ERROR(operation(std::move(name), mapItem.second));
        }
    }

    void ConfigurationSetParser::ParseSequence(const AppInstaller::YAML::Node& node, ConfigurationField field, bool required, std::optional<Node::Type> elementType, std::function<void(const AppInstaller::YAML::Node&)> operation)
    {
        const Node& sequenceNode = CHECK_ERROR(GetAndEnsureField(node, field, required, Node::Type::Sequence));
        if (!sequenceNode)
        {
            return;
        }

        ParseSequence(sequenceNode, GetConfigurationFieldName(field), elementType, operation);
    }

    void ConfigurationSetParser::ParseSequence(const AppInstaller::YAML::Node& node, std::string_view nameForErrors, std::optional<Node::Type> elementType, std::function<void(const AppInstaller::YAML::Node&)> operation)
    {
        std::ostringstream strstr;
        strstr << nameForErrors;
        size_t index = 0;

        for (const Node& item : node.Sequence())
        {
            if (elementType && item.GetType() != elementType.value())
            {
                strstr << '[' << index << ']';
                FIELD_TYPE_ERROR(strstr.str(), item.Mark());
            }
            index++;

            CHECK_ERROR(operation(item));
        }
    }

    std::unique_ptr<ConfigurationSetParser> ConfigurationSetParser::GetSchemaVersionFromOldFormat(AppInstaller::YAML::Node& document, std::string& schemaVersionString)
    {
        Node& propertiesNode = document[GetConfigurationFieldName(ConfigurationField::Properties)];
        if (!propertiesNode)
        {
            AICLI_LOG(Config, Error, << "No properties");
            // Even though this is for the "older" format, if there is no properties entry then give an error for the newer format since this is probably neither.
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_MISSING_FIELD, GetConfigurationFieldName(ConfigurationField::Schema));
        }
        else if (!propertiesNode.IsMap())
        {
            AICLI_LOG(Config, Error, << "Invalid properties type");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, GetConfigurationFieldName(ConfigurationField::Properties), propertiesNode.Mark());
        }

        Node& versionNode = propertiesNode[GetConfigurationFieldName(ConfigurationField::ConfigurationVersion)];
        if (!versionNode)
        {
            AICLI_LOG(Config, Error, << "No configuration version");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_MISSING_FIELD, GetConfigurationFieldName(ConfigurationField::ConfigurationVersion));
        }
        else if (!versionNode.IsScalar())
        {
            AICLI_LOG(Config, Error, << "Invalid configuration version type");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, GetConfigurationFieldName(ConfigurationField::ConfigurationVersion), versionNode.Mark());
        }

        schemaVersionString = versionNode.as<std::string>();
        return {};
    }

    void ConfigurationSetParser::GetStringValueForUnit(const Node& node, ConfigurationField field, bool required, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(const hstring& value))
    {
        const Node& valueNode = CHECK_ERROR(GetAndEnsureField(node, field, required, Node::Type::Scalar));

        if (valueNode)
        {
            hstring value{ valueNode.as<std::wstring>() };
            FIELD_MISSING_ERROR_IF(value.empty() && required, GetConfigurationFieldName(field));

            (unit->*propertyFunction)(std::move(value));
        }
    }

    void ConfigurationSetParser::GetStringArrayForUnit(const Node& node, ConfigurationField field, bool required, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(std::vector<hstring>&& value))
    {
        std::vector<hstring> arrayValue;
        CHECK_ERROR(ParseSequence(node, field, required, Node::Type::Scalar, [&](const AppInstaller::YAML::Node& item)
            {
                arrayValue.emplace_back(item.as<std::wstring>());
            }));

        if (!arrayValue.empty())
        {
            (unit->*propertyFunction)(std::move(arrayValue));
        }
    }

    void ConfigurationSetParser::ValidateType(ConfigurationUnit* unit, const Node& unitNode, ConfigurationField typeField, bool moveModuleNameToMetadata, bool moduleNameRequiredInType)
    {
        QualifiedResourceName qualifiedName{ unit->Type() };

        const Node& typeNode = CHECK_ERROR(GetAndEnsureField(unitNode, typeField, true, Node::Type::Scalar));
        FIELD_VALUE_ERROR_IF(qualifiedName.Resource.empty(), GetConfigurationFieldName(typeField), ConvertToUTF8(unit->Type()), typeNode.Mark());

        if (!qualifiedName.Module.empty())
        {
            // If the module is provided in both the resource name and the directives, ensure that it matches
            hstring moduleDirectiveFieldName = GetConfigurationFieldNameHString(ConfigurationField::ModuleDirective);
            auto moduleDirective = unit->Metadata().TryLookup(moduleDirectiveFieldName);
            if (moduleDirective)
            {
                auto moduleProperty = moduleDirective.try_as<Windows::Foundation::IPropertyValue>();
                FIELD_TYPE_ERROR_IF(!moduleProperty, GetConfigurationFieldName(ConfigurationField::ModuleDirective), unitNode.Mark());
                FIELD_TYPE_ERROR_IF(moduleProperty.Type() != Windows::Foundation::PropertyType::String, GetConfigurationFieldName(ConfigurationField::ModuleDirective), unitNode.Mark());
                hstring moduleValue = moduleProperty.GetString();
                FIELD_VALUE_ERROR_IF(qualifiedName.Module != moduleValue, GetConfigurationFieldName(ConfigurationField::ModuleDirective), ConvertToUTF8(moduleValue), unitNode.Mark());
            }
            else if (moveModuleNameToMetadata)
            {
                unit->Metadata().Insert(moduleDirectiveFieldName, Windows::Foundation::PropertyValue::CreateString(qualifiedName.Module));
            }

            if (moveModuleNameToMetadata)
            {
                // Set the unit name to be just the resource portion
                unit->Type(qualifiedName.Resource);
            }
        }
        else if (moduleNameRequiredInType)
        {
            FIELD_VALUE_ERROR(GetConfigurationFieldName(typeField), ConvertToUTF8(unit->Type()), typeNode.Mark());
        }
    }

    void ConfigurationSetParser::ParseObject(const Node& node, ConfigurationField fieldForErrors, Windows::Foundation::PropertyType type, Windows::Foundation::IInspectable& result)
    {
        try
        {
            Windows::Foundation::IInspectable object = GetIInspectableFromNode(node);
            FIELD_VALUE_ERROR_IF(!IsValidObjectType(object, type), GetConfigurationFieldName(fieldForErrors), node.as<std::string>(), node.Mark());
            result = std::move(object);
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION();
            FIELD_VALUE_ERROR(GetConfigurationFieldName(fieldForErrors), node.as<std::string>(), node.Mark());
        }
    }

    void ConfigurationSetParser::ExtractSecurityContext(implementation::ConfigurationUnit* unit, SecurityContext defaultContext)
    {
        THROW_HR_IF_NULL(E_POINTER, unit);

        SecurityContext computedContext = defaultContext;

        Windows::Foundation::Collections::ValueSet metadata = unit->Metadata();
        auto securityContext = TryLookupProperty(metadata, ConfigurationField::SecurityContextMetadata, Windows::Foundation::PropertyType::String);
        if (securityContext)
        {
            TryParseSecurityContext(securityContext.GetString(), computedContext);
            metadata.Remove(GetConfigurationFieldNameHString(ConfigurationField::SecurityContextMetadata));
        }

        unit->EnvironmentInternal().Context(computedContext);
    }
}
