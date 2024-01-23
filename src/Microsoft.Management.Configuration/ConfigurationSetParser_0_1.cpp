// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser_0_1.h"
#include "ParsingMacros.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

#include <sstream>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;

    void ConfigurationSetParser_0_1::Parse()
    {
        std::vector<Configuration::ConfigurationUnit> units;
        const Node& properties = m_document[GetFieldName(FieldName::Properties)];
        ParseConfigurationUnitsFromField(properties, FieldName::Assertions, ConfigurationUnitIntent::Assert, units);
        ParseConfigurationUnitsFromField(properties, FieldName::Parameters, ConfigurationUnitIntent::Inform, units);
        ParseConfigurationUnitsFromField(properties, FieldName::Resources, ConfigurationUnitIntent::Apply, units);

        m_configurationSet = make_self<wil::details::module_count_wrapper<implementation::ConfigurationSet>>();
        m_configurationSet->Units(std::move(units));
        m_configurationSet->SchemaVersion(GetSchemaVersion());
    }

    hstring ConfigurationSetParser_0_1::GetSchemaVersion()
    {
        static hstring s_schemaVersion{ L"0.1" };
        return s_schemaVersion;
    }

    void ConfigurationSetParser_0_1::ParseConfigurationUnitsFromField(const Node& document, FieldName field, ConfigurationUnitIntent intent, std::vector<Configuration::ConfigurationUnit>& result)
    {
        ParseSequence(document, field, false, Node::Type::Mapping, [&](const Node& item)
            {
                auto configurationUnit = make_self<wil::details::module_count_wrapper<ConfigurationUnit>>();
                ParseConfigurationUnit(configurationUnit.get(), item, intent);
                result.emplace_back(*configurationUnit);
            });
    }

    void ConfigurationSetParser_0_1::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode, ConfigurationUnitIntent intent)
    {
        CHECK_ERROR(GetStringValueForUnit(unitNode, FieldName::Resource, true, unit, &ConfigurationUnit::Type));
        CHECK_ERROR(GetStringValueForUnit(unitNode, FieldName::Id, false, unit, &ConfigurationUnit::Identifier));
        unit->Intent(intent);
        CHECK_ERROR(GetStringArrayForUnit(unitNode, FieldName::DependsOn, false, unit, &ConfigurationUnit::Dependencies));
        CHECK_ERROR(ParseValueSet(unitNode, FieldName::Directives, false, unit->Metadata()));
        CHECK_ERROR(ParseValueSet(unitNode, FieldName::Settings, false, unit->Settings()));
    }
}
