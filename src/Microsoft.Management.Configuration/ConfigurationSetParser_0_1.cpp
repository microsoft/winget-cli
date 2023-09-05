// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser_0_1.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

#include <sstream>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;

#define CHECK_ERROR(_op_) (_op_); if (FAILED(m_result)) { return; }

#define FIELD_TYPE_ERROR(_field_,_mark_) SetError(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, (_field_), (_mark_)); return
#define FIELD_TYPE_ERROR_IF(_condition_,_field_,_mark_) if (_condition_) { FIELD_TYPE_ERROR(_field_,_mark_); }

#define FIELD_MISSING_ERROR(_field_) SetError(WINGET_CONFIG_ERROR_MISSING_FIELD, (_field_)); return
#define FIELD_MISSING_ERROR_IF(_condition_,_field_) if (_condition_) { FIELD_MISSING_ERROR(_field_); }

    namespace
    {
        Windows::Foundation::IInspectable GetIInspectableFromNode(const Node& node);

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
    }

    std::vector<Configuration::ConfigurationUnit> ConfigurationSetParser_0_1::GetConfigurationUnits()
    {
        std::vector<Configuration::ConfigurationUnit> result;
        const Node& properties = m_document[GetFieldName(FieldName::Properties)];
        ParseConfigurationUnitsFromSubsection(properties, "assertions", ConfigurationUnitIntent::Assert, result);
        ParseConfigurationUnitsFromSubsection(properties, "parameters", ConfigurationUnitIntent::Inform, result);
        ParseConfigurationUnitsFromSubsection(properties, "resources", ConfigurationUnitIntent::Apply, result);
        // TODO: Additional semantic validation?
        return result;
    }

    hstring ConfigurationSetParser_0_1::GetSchemaVersion()
    {
        static hstring s_schemaVersion{ L"0.1" };
        return s_schemaVersion;
    }

    void ConfigurationSetParser_0_1::ParseConfigurationUnitsFromSubsection(const Node& document, std::string_view subsection, ConfigurationUnitIntent intent, std::vector<Configuration::ConfigurationUnit>& result)
    {
        if (FAILED(m_result))
        {
            return;
        }

        Node subsectionNode = document[subsection];

        if (!subsectionNode.IsDefined())
        {
            return;
        }

        FIELD_TYPE_ERROR_IF(!subsectionNode.IsSequence(), subsection, subsectionNode.Mark());

        std::ostringstream strstr;
        strstr << subsection;
        size_t index = 0;

        for (const Node& item : subsectionNode.Sequence())
        {
            if (!item.IsMap())
            {
                strstr << '[' << index << ']';
                FIELD_TYPE_ERROR(strstr.str(), item.Mark());
            }
            index++;

            auto configurationUnit = make_self<wil::details::module_count_wrapper<ConfigurationUnit>>();

            ParseConfigurationUnit(configurationUnit.get(), item, intent);

            result.emplace_back(*configurationUnit);
        }
    }

    void ConfigurationSetParser_0_1::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode, ConfigurationUnitIntent intent)
    {
        CHECK_ERROR(GetStringValueForUnit(unitNode, GetFieldName(FieldName::Resource), true, unit, &ConfigurationUnit::Type));
        CHECK_ERROR(GetStringValueForUnit(unitNode, "id", false, unit, &ConfigurationUnit::Identifier));
        unit->Intent(intent);
        CHECK_ERROR(GetStringArrayForUnit(unitNode, "dependsOn", unit, &ConfigurationUnit::Dependencies));
        CHECK_ERROR(GetValueSet(unitNode, "directives", false, unit->Metadata()));
        CHECK_ERROR(GetValueSet(unitNode, "settings", false, unit->Settings()));
    }

    void ConfigurationSetParser_0_1::GetStringValueForUnit(const Node& item, std::string_view valueName, bool required, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(const hstring& value))
    {
        const Node& valueNode = item[valueName];

        if (valueNode)
        {
            FIELD_TYPE_ERROR_IF(!valueNode.IsScalar(), valueName, valueNode.Mark());
        }
        else
        {
            FIELD_MISSING_ERROR_IF(required, valueName);
            return;
        }

        hstring value{ valueNode.as<std::wstring>() };
        FIELD_MISSING_ERROR_IF(value.empty() && required, valueName);

        (unit->*propertyFunction)(std::move(value));
    }

    void ConfigurationSetParser_0_1::GetStringArrayForUnit(const Node& item, std::string_view arrayName, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(std::vector<hstring>&& value))
    {
        const Node& arrayNode = item[arrayName];

        if (!arrayNode)
        {
            return;
        }

        FIELD_TYPE_ERROR_IF(!arrayNode.IsSequence(), arrayName, arrayNode.Mark());

        std::vector<hstring> arrayValue;

        std::ostringstream strstr;
        strstr << arrayName;
        size_t index = 0;

        for (const Node& arrayItem : arrayNode.Sequence())
        {
            if (!arrayItem.IsScalar())
            {
                strstr << '[' << index << ']';
                FIELD_TYPE_ERROR(strstr.str(), arrayItem.Mark());
            }
            index++;

            arrayValue.emplace_back(arrayItem.as<std::wstring>());
        }

        (unit->*propertyFunction)(std::move(arrayValue));
    }

    void ConfigurationSetParser_0_1::GetValueSet(const Node& item, std::string_view mapName, bool required, const Windows::Foundation::Collections::ValueSet& valueSet)
    {
        const Node& mapNode = item[mapName];

        if (mapNode)
        {
            FIELD_TYPE_ERROR_IF(!mapNode.IsMap(), mapName, mapNode.Mark());
        }
        else
        {
            FIELD_MISSING_ERROR_IF(required, mapName);
            return;
        }

        FillValueSetFromMap(mapNode, valueSet);
    }
}
