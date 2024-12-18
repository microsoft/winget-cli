// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser_0_3.h"
#include "ParsingMacros.h"
#include "ArgumentValidation.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

#include <sstream>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;

    void ConfigurationSetParser_0_3::Parse()
    {
        auto result = make_self<implementation::ConfigurationSet>();

        CHECK_ERROR(ParseValueSet(m_document, ConfigurationField::Metadata, false, result->Metadata()));

        m_setEnvironment = make_self<implementation::ConfigurationEnvironment>();
        CHECK_ERROR(ExtractEnvironmentFromMetadata(result->Metadata(), *m_setEnvironment));

        CHECK_ERROR(ParseParameters(result));
        CHECK_ERROR(ParseValueSet(m_document, ConfigurationField::Variables, false, result->Variables()));

        std::vector<Configuration::ConfigurationUnit> units;
        CHECK_ERROR(ParseConfigurationUnitsFromField(m_document, ConfigurationField::Resources, *m_setEnvironment, units));
        result->Units(std::move(units));

        result->SchemaVersion(GetSchemaVersion());
        m_configurationSet = std::move(result);
    }

    hstring ConfigurationSetParser_0_3::GetSchemaVersion()
    {
        static hstring s_schemaVersion{ L"0.3" };
        return s_schemaVersion;
    }

    void ConfigurationSetParser_0_3::SetDocument(AppInstaller::YAML::Node&& document)
    {
        m_document = std::move(document);
    }

    void ConfigurationSetParser_0_3::ParseParameters(ConfigurationSetParser::ConfigurationSetPtr& set)
    {
        std::vector<Configuration::ConfigurationParameter> parameters;

        ParseMapping(m_document, ConfigurationField::Parameters, false, Node::Type::Mapping, [&](std::string name, const Node& item)
            {
                auto parameter = make_self<wil::details::module_count_wrapper<ConfigurationParameter>>();
                CHECK_ERROR(ParseParameter(parameter.get(), item));
                parameter->Name(hstring{ AppInstaller::Utility::ConvertToUTF16(name) });
                parameters.emplace_back(*parameter);
            });

        set->Parameters(std::move(parameters));
    }

    void ConfigurationSetParser_0_3::ParseParameter(ConfigurationParameter* parameter, const AppInstaller::YAML::Node& node)
    {
        CHECK_ERROR(ParseParameterType(parameter, node));
        CHECK_ERROR(ParseValueSet(node, ConfigurationField::Metadata, false, parameter->Metadata()));
        CHECK_ERROR(GetStringValueForParameter(node, ConfigurationField::Description, parameter, &ConfigurationParameter::Description));

        Windows::Foundation::PropertyType parameterType = parameter->Type();
        CHECK_ERROR(ParseObjectValueForParameter(node, ConfigurationField::DefaultValue, parameterType, parameter, &ConfigurationParameter::DefaultValue));

        std::vector<Windows::Foundation::IInspectable> allowedValues;

        CHECK_ERROR(ParseSequence(node, ConfigurationField::AllowedValues, false, std::nullopt, [&](const Node& item)
            {
                Windows::Foundation::IInspectable object;
                CHECK_ERROR(ParseObject(item, ConfigurationField::AllowedValues, parameterType, object));
                allowedValues.emplace_back(std::move(object));
            }));

        if (!allowedValues.empty())
        {
            parameter->AllowedValues(std::move(allowedValues));
        }

        if (IsLengthType(parameterType))
        {
            CHECK_ERROR(GetUInt32ValueForParameter(node, ConfigurationField::MinimumLength, parameter, &ConfigurationParameter::MinimumLength));
            CHECK_ERROR(GetUInt32ValueForParameter(node, ConfigurationField::MaximumLength, parameter, &ConfigurationParameter::MaximumLength));
        }
        else
        {
            CHECK_ERROR(EnsureFieldAbsent(node, ConfigurationField::MinimumLength));
            CHECK_ERROR(EnsureFieldAbsent(node, ConfigurationField::MaximumLength));
        }

        if (IsComparableType(parameterType))
        {
            CHECK_ERROR(ParseObjectValueForParameter(node, ConfigurationField::MinimumValue, parameterType, parameter, &ConfigurationParameter::MinimumValue));
            CHECK_ERROR(ParseObjectValueForParameter(node, ConfigurationField::MaximumValue, parameterType, parameter, &ConfigurationParameter::MaximumValue));
        }
        else
        {
            CHECK_ERROR(EnsureFieldAbsent(node, ConfigurationField::MinimumValue));
            CHECK_ERROR(EnsureFieldAbsent(node, ConfigurationField::MaximumValue));
        }
    }

    void ConfigurationSetParser_0_3::ParseParameterType(ConfigurationParameter* parameter, const AppInstaller::YAML::Node& node)
    {
        const Node& typeNode = CHECK_ERROR(GetAndEnsureField(node, ConfigurationField::Type, true, Node::Type::Scalar));
        std::string typeValue = typeNode.as<std::string>();
        auto parsedType = ParseWindowsFoundationPropertyType(typeValue);

        if (parsedType)
        {
            parameter->Type(parsedType->first);
            parameter->IsSecure(parsedType->second);
        }
        else
        {
            FIELD_VALUE_ERROR(GetConfigurationFieldName(ConfigurationField::Type), typeValue, typeNode.Mark());
        }
    }

    void ConfigurationSetParser_0_3::GetStringValueForParameter(
        const Node& node,
        ConfigurationField field,
        ConfigurationParameter* parameter,
        void(ConfigurationParameter::* propertyFunction)(const hstring& value))
    {
        const Node& valueNode = CHECK_ERROR(GetAndEnsureField(node, field, false, Node::Type::Scalar));

        if (valueNode)
        {
            (parameter->*propertyFunction)(hstring{ valueNode.as<std::wstring>() });
        }
    }

    void ConfigurationSetParser_0_3::GetUInt32ValueForParameter(
        const AppInstaller::YAML::Node& node,
        ConfigurationField field,
        ConfigurationParameter* parameter,
        void(ConfigurationParameter::* propertyFunction)(uint32_t value))
    {
        const Node& valueNode = CHECK_ERROR(GetAndEnsureField(node, field, false, Node::Type::Scalar));

        if (valueNode)
        {
            int64_t value = valueNode.as<int64_t>();
            if (value < 0 || value > static_cast<int64_t>(std::numeric_limits<uint32_t>::max()))
            {
                FIELD_VALUE_ERROR(GetConfigurationFieldName(field), valueNode.as<std::string>(), valueNode.Mark());
            }
            (parameter->*propertyFunction)(static_cast<uint32_t>(value));
        }
    }

    void ConfigurationSetParser_0_3::ParseObjectValueForParameter(
        const AppInstaller::YAML::Node& node,
        ConfigurationField field,
        Windows::Foundation::PropertyType type,
        ConfigurationParameter* parameter,
        void(ConfigurationParameter::* propertyFunction)(const Windows::Foundation::IInspectable& value))
    {
        const Node& valueNode = CHECK_ERROR(GetAndEnsureField(node, field, false, std::nullopt));

        if (valueNode)
        {
            Windows::Foundation::IInspectable valueObject;
            CHECK_ERROR(ParseObject(valueNode, field, type, valueObject));

            (parameter->*propertyFunction)(valueObject);
        }
    }

    void ConfigurationSetParser_0_3::ParseConfigurationUnitsFromField(const Node& document, ConfigurationField field, const ConfigurationEnvironment& defaultEnvironment, std::vector<Configuration::ConfigurationUnit>& result)
    {
        ParseSequence(document, field, false, Node::Type::Mapping, [&](const Node& item)
            {
                auto configurationUnit = make_self<ConfigurationUnit>();
                ParseConfigurationUnit(configurationUnit.get(), item, defaultEnvironment);
                result.emplace_back(*configurationUnit);
            });
    }

    void ConfigurationSetParser_0_3::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode, const ConfigurationEnvironment& defaultEnvironment)
    {
        // Set unknown intent as the new schema doesn't express it directly
        unit->Intent(ConfigurationUnitIntent::Unknown);

        CHECK_ERROR(GetStringValueForUnit(unitNode, ConfigurationField::Name, true, unit, &ConfigurationUnit::Identifier));
        CHECK_ERROR(GetStringValueForUnit(unitNode, ConfigurationField::Type, true, unit, &ConfigurationUnit::Type));
        CHECK_ERROR(ParseValueSet(unitNode, ConfigurationField::Metadata, false, unit->Metadata()));
        CHECK_ERROR(ExtractEnvironmentForUnit(unit, defaultEnvironment));
        CHECK_ERROR(ValidateType(unit, unitNode, ConfigurationField::Type, false, true));
        CHECK_ERROR(GetStringArrayForUnit(unitNode, ConfigurationField::DependsOn, false, unit, &ConfigurationUnit::Dependencies));

        // Regardless of being a group or not, parse the settings.
        CHECK_ERROR(ParseValueSet(unitNode, ConfigurationField::Properties, false, unit->Settings()));

        if (ShouldConvertToGroup(unit))
        {
            unit->IsGroup(true);

            // TODO: The PS DSC v3 POR looks like it supports each group defining a new schema to be used for its group items.
            //       Consider supporting that in the future; but for now just use the same schema for everything.
            const Node& propertiesNode = GetAndEnsureField(unitNode, ConfigurationField::Properties, false, Node::Type::Mapping);
            if (propertiesNode)
            {
                std::vector<Configuration::ConfigurationUnit> units;
                CHECK_ERROR(ParseConfigurationUnitsFromField(propertiesNode, ConfigurationField::Resources, unit->EnvironmentInternal(), units));
                unit->Units(std::move(units));
            }
        }
    }

    bool ConfigurationSetParser_0_3::ShouldConvertToGroup(ConfigurationUnit* unit)
    {
        // Allow the metadata to inform us that we should treat it as a group, including preventing a known type from being treated as one.
        auto isGroupObject = unit->Metadata().TryLookup(GetConfigurationFieldNameHString(ConfigurationField::IsGroupMetadata));
        if (isGroupObject)
        {
            auto isGroupProperty = isGroupObject.try_as<Windows::Foundation::IPropertyValue>();
            if (isGroupProperty && isGroupProperty.Type() == Windows::Foundation::PropertyType::Boolean)
            {
                return isGroupProperty.GetBoolean();
            }
        }

        // TODO: Check for known types

        return false;
    }

    void ConfigurationSetParser_0_3::ExtractEnvironmentFromMetadata(const Windows::Foundation::Collections::ValueSet& metadata, ConfigurationEnvironment& targetEnvironment, const ConfigurationEnvironment* defaultEnvironment)
    {

    }

    void ConfigurationSetParser_0_3::ExtractEnvironmentForUnit(ConfigurationUnit* unit, const ConfigurationEnvironment& defaultEnvironment)
    {
        // Get unnested the security context
        ExtractSecurityContext(unit, defaultEnvironment.Context());

        // Get nested environment
        ExtractEnvironmentFromMetadata(unit->Metadata(), unit->EnvironmentInternal(), &defaultEnvironment);
    }

    std::optional<std::pair<Windows::Foundation::PropertyType, bool>> ParseWindowsFoundationPropertyType(std::string_view value)
    {
        if (value == "string")
        {
            return std::make_pair(Windows::Foundation::PropertyType::String, false);
        }
        else if (value == "securestring")
        {
            return std::make_pair(Windows::Foundation::PropertyType::String, true);
        }
        else if (value == "int")
        {
            return std::make_pair(Windows::Foundation::PropertyType::Int64, false);
        }
        else if (value == "bool")
        {
            return std::make_pair(Windows::Foundation::PropertyType::Boolean, false);
        }
        else if (value == "object")
        {
            return std::make_pair(Windows::Foundation::PropertyType::Inspectable, false);
        }
        else if (value == "secureobject")
        {
            return std::make_pair(Windows::Foundation::PropertyType::Inspectable, true);
        }
        else if (value == "array")
        {
            return std::make_pair(Windows::Foundation::PropertyType::InspectableArray, false);
        }

        // TODO: Consider supporting an expanded set of type strings
        return std::nullopt;
    }

    std::string_view ToString(Windows::Foundation::PropertyType value, bool isSecure)
    {
        switch (value)
        {
        case Windows::Foundation::PropertyType::Int16:
        case Windows::Foundation::PropertyType::Int32:
        case Windows::Foundation::PropertyType::Int64:
            return "int"sv;
        case Windows::Foundation::PropertyType::Boolean:
            return "bool"sv;
        case Windows::Foundation::PropertyType::String:
            return isSecure ? "securestring"sv : "string"sv;
        case Windows::Foundation::PropertyType::Inspectable:
            return isSecure ? "secureobject"sv : "object"sv;
        case Windows::Foundation::PropertyType::InspectableArray:
            return "array"sv;
        default:
            return {};
        }
    }
}
