// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser_0_1.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

#include <sstream>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        using namespace AppInstaller::YAML;

        void GetStringValueForUnit(const Node& item, std::string_view valueName, bool required, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(const hstring& value))
        {
            const Node& valueNode = item[valueName];

            if (valueNode)
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, !valueNode.IsScalar());
            }
            else
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, required);
                return;
            }

            (unit->*propertyFunction)(hstring{ valueNode.as<std::wstring>() });
        }

        void GetStringArrayForUnit(const Node& item, std::string_view arrayName, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(std::vector<hstring>&& value))
        {
            const Node& arrayNode = item[arrayName];

            if (!arrayNode)
            {
                return;
            }

            THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, !arrayNode.IsSequence());

            std::vector<hstring> arrayValue;

            for (const Node& arrayItem : arrayNode.Sequence())
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, !arrayItem.IsScalar());
                arrayValue.emplace_back(arrayItem.as<std::wstring>());
            }

            (unit->*propertyFunction)(std::move(arrayValue));
        }

        Windows::Foundation::IInspectable GetIInspectableFromNode(const Node& node);

        // Returns the appropriate IPropertyValue for the given node, which is assumed to be a scalar.
        Windows::Foundation::IInspectable GetPropertyValueFromScalar(const Node& node)
        {
            // TODO: Use the tag to determine the type of property to create.
            return Windows::Foundation::PropertyValue::CreateString(node.as<std::wstring>());
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

        // Fills the ValueSet from the given node, which is assumed to be a map.
        void FillValueSetFromMap(const Node& mapNode, const Windows::Foundation::Collections::ValueSet& valueSet)
        {
            for (const auto& mapItem : mapNode.Mapping())
            {
                // Insert returns true if it replaces an existing key, and that indicates an invalid map.
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, valueSet.Insert(mapItem.first.as<std::wstring>(), GetIInspectableFromNode(mapItem.second)));
            }
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

        void GetValueSet(const Node& mapNode, bool required, const Windows::Foundation::Collections::ValueSet& valueSet)
        {
            if (mapNode)
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, !mapNode.IsMap());
            }
            else
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, required);
                return;
            }

            FillValueSetFromMap(mapNode, valueSet);
        }

        void ParseConfigurationUnitsFromSubsection(const Node& document, std::string_view subsection, ConfigurationUnitIntent intent, std::vector<Configuration::ConfigurationUnit>& result)
        {
            Node subsectionNode = document[subsection];

            if (!subsectionNode.IsDefined())
            {
                return;
            }

            THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, !subsectionNode.IsSequence());

            for (const Node& item : subsectionNode.Sequence())
            {
                THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE, !item.IsMap());

                auto configurationUnit = make_self<wil::details::module_count_wrapper<ConfigurationUnit>>();

                GetStringValueForUnit(item, "resource", true, configurationUnit.get(), &ConfigurationUnit::UnitName);
                GetStringValueForUnit(item, "id", false, configurationUnit.get(), &ConfigurationUnit::Identifier);
                configurationUnit->Intent(intent);
                GetStringArrayForUnit(item, "dependsOn", configurationUnit.get(), &ConfigurationUnit::Dependencies);
                GetValueSet(item["directives"], false, configurationUnit->Directives());
                GetValueSet(item["settings"], false, configurationUnit->Settings());

                result.emplace_back(*configurationUnit);
            }
        }
    }

    std::vector<Configuration::ConfigurationUnit> ConfigurationSetParser_0_1::GetConfigurationUnits()
    {
        std::vector<Configuration::ConfigurationUnit> result;
        const Node& properties = m_document[NodeName_Properties];
        ParseConfigurationUnitsFromSubsection(properties, "assertions", ConfigurationUnitIntent::Assert, result);
        ParseConfigurationUnitsFromSubsection(properties, "parameters", ConfigurationUnitIntent::Inform, result);
        ParseConfigurationUnitsFromSubsection(properties, "resources", ConfigurationUnitIntent::Apply, result);
        return result;
    }
}
