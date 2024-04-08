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
        auto result = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();

        CHECK_ERROR(ParseValueSet(m_document, ConfigurationField::Metadata, false, result->Metadata()));
        CHECK_ERROR(ParseParameters(result));
        CHECK_ERROR(ParseValueSet(m_document, ConfigurationField::Variables, false, result->Variables()));

        std::vector<Configuration::ConfigurationUnit> units;
        CHECK_ERROR(ParseConfigurationUnitsFromField(m_document, ConfigurationField::Resources, units));
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
            FIELD_VALUE_ERROR(GetConfigurationFieldName(ConfigurationField::Type), typeValue, typeNode.Mark());
        }

        // TODO: Consider supporting an expanded set of type strings
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

    void ConfigurationSetParser_0_3::ParseConfigurationUnitsFromField(const Node& document, ConfigurationField field, std::vector<Configuration::ConfigurationUnit>& result)
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

        CHECK_ERROR(GetStringValueForUnit(unitNode, ConfigurationField::Name, true, unit, &ConfigurationUnit::Identifier));
        CHECK_ERROR(GetStringValueForUnit(unitNode, ConfigurationField::Type, true, unit, &ConfigurationUnit::Type));
        CHECK_ERROR(ParseValueSet(unitNode, ConfigurationField::Metadata, false, unit->Metadata()));
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
                CHECK_ERROR(ParseConfigurationUnitsFromField(propertiesNode, ConfigurationField::Resources, units));
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
}
