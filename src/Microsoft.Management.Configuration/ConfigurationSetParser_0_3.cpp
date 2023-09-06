// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser_0_3.h"
#include "ParsingMacros.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

#include <sstream>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;

    void ConfigurationSetParser_0_3::Parse()
    {
        auto result = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();

        CHECK_ERROR(ParseValueSet(m_document, FieldName::Metadata, false, result->Metadata()));
        CHECK_ERROR(ParseParameters(result));
        CHECK_ERROR(ParseValueSet(m_document, FieldName::Variables, false, result->Variables()));

        std::vector<Configuration::ConfigurationUnit> units;
        CHECK_ERROR(ParseConfigurationUnitsFromField(m_document, FieldName::Resources, units));
        result->Units(std::move(units));

        result->SchemaVersion(GetSchemaVersion());
        m_configurationSet = std::move(result);
    }

    hstring ConfigurationSetParser_0_3::GetSchemaVersion()
    {
        static hstring s_schemaVersion{ L"0.3" };
        return s_schemaVersion;
    }

    void ConfigurationSetParser_0_3::ParseParameters(ConfigurationSetParser::ConfigurationSetPtr& set)
    {
        std::vector<Configuration::ConfigurationParameter> parameters;

        ParseMapping(m_document, FieldName::Parameters, false, Node::Type::Mapping, [&](std::string name, const Node& item)
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
        CHECK_ERROR(ParseValueSet(node, FieldName::Metadata, false, parameter->Metadata()));
        CHECK_ERROR(GetStringValueForParameter(node, FieldName::Description, parameter, &ConfigurationParameter::Description));

        // TODO: Parse the remaining parameter fields when we implement parameter/variable support.
        //       We currently parse enough to detect an attempt to use parameters which will be rejected on processing.
        //  defaultValue
        //  allowedValues
        //  minLength
        //  maxLength
        //  minValue
        //  maxValue
    }

    void ConfigurationSetParser_0_3::ParseParameterType(ConfigurationParameter* parameter, const AppInstaller::YAML::Node& node)
    {
        const Node& typeNode = CHECK_ERROR(GetAndEnsureField(node, FieldName::Type, true, Node::Type::Scalar));
        std::string typeValue = typeNode.as<std::string>();

        if (typeValue == "string")
        {
            parameter->Type(Windows::Foundation::PropertyType::String);
        }
        else if (typeValue == "securestring")
        {
            parameter->Type(Windows::Foundation::PropertyType::String);
            parameter->IsSecure(true);
        }
        else if (typeValue == "int")
        {
            parameter->Type(Windows::Foundation::PropertyType::Int64);
        }
        else if (typeValue == "bool")
        {
            parameter->Type(Windows::Foundation::PropertyType::Boolean);
        }
        else if (typeValue == "object")
        {
            parameter->Type(Windows::Foundation::PropertyType::Inspectable);
        }
        else if (typeValue == "secureobject")
        {
            parameter->Type(Windows::Foundation::PropertyType::Inspectable);
            parameter->IsSecure(true);
        }
        else if (typeValue == "array")
        {
            parameter->Type(Windows::Foundation::PropertyType::InspectableArray);
        }
        else
        {
            FIELD_VALUE_ERROR(GetFieldName(FieldName::Type), typeValue, typeNode.Mark());
        }

        // TODO: Consider supporting an expanded set of type strings
    }

    void ConfigurationSetParser_0_3::GetStringValueForParameter(const Node& node, FieldName field, ConfigurationParameter* parameter, void(ConfigurationParameter::* propertyFunction)(const hstring& value))
    {
        const Node& valueNode = CHECK_ERROR(GetAndEnsureField(node, field, false, Node::Type::Scalar));

        if (valueNode)
        {
            (parameter->*propertyFunction)(hstring{ valueNode.as<std::wstring>() });
        }
    }

    void ConfigurationSetParser_0_3::ParseConfigurationUnitsFromField(const Node& document, FieldName field, std::vector<Configuration::ConfigurationUnit>& result)
    {
        ParseSequence(document, field, false, Node::Type::Mapping, [&](const Node& item)
            {
                auto configurationUnit = make_self<wil::details::module_count_wrapper<ConfigurationUnit>>();
                ParseConfigurationUnit(configurationUnit.get(), item);
                result.emplace_back(*configurationUnit);
            });
    }

    void ConfigurationSetParser_0_3::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode)
    {
        // Set unknown intent as the new schema doesn't express it directly
        unit->Intent(ConfigurationUnitIntent::Unknown);

        CHECK_ERROR(GetStringValueForUnit(unitNode, FieldName::Name, true, unit, &ConfigurationUnit::Identifier));
        CHECK_ERROR(GetStringValueForUnit(unitNode, FieldName::Type, true, unit, &ConfigurationUnit::Type));
        CHECK_ERROR(ParseValueSet(unitNode, FieldName::Metadata, false, unit->Metadata()));
        CHECK_ERROR(ValidateType(unit, unitNode, FieldName::Type, false, true));
        CHECK_ERROR(GetStringArrayForUnit(unitNode, FieldName::DependsOn, false, unit, &ConfigurationUnit::Dependencies));

        // Regardless of being a group or not, parse the settings.
        CHECK_ERROR(ParseValueSet(unitNode, FieldName::Properties, false, unit->Settings()));

        if (ShouldConvertToGroup(unit))
        {
            unit->IsGroup(true);

            // TODO: The PS DSC v3 POR looks like it supports each group defining a new schema to be used for its group items.
            //       Consider supporting that in the future; but for now just use the same schema for everything.
            const Node& propertiesNode = GetAndEnsureField(unitNode, FieldName::Properties, false, Node::Type::Mapping);
            if (propertiesNode)
            {
                std::vector<Configuration::ConfigurationUnit> units;
                CHECK_ERROR(ParseConfigurationUnitsFromField(propertiesNode, FieldName::Resources, units));
                unit->Units(std::move(units));
            }
        }
    }

    bool ConfigurationSetParser_0_3::ShouldConvertToGroup(ConfigurationUnit* unit)
    {
        // Allow the metadata to inform us that we should treat it as a group, including preventing a known type from being treated as one.
        auto isGroupObject = unit->Metadata().TryLookup(GetFieldNameHString(FieldName::IsGroupMetadata));
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
}
