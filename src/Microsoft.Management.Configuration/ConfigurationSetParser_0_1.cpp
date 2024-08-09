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
        const Node& properties = m_document[GetConfigurationFieldName(ConfigurationField::Properties)];
        ParseConfigurationUnitsFromField(properties, ConfigurationField::Assertions, ConfigurationUnitIntent::Assert, units);
        ParseConfigurationUnitsFromField(properties, ConfigurationField::Parameters, ConfigurationUnitIntent::Inform, units);
        ParseConfigurationUnitsFromField(properties, ConfigurationField::Resources, ConfigurationUnitIntent::Apply, units);

        m_configurationSet = make_self<implementation::ConfigurationSet>();
        m_configurationSet->Units(std::move(units));
        m_configurationSet->SchemaVersion(GetSchemaVersion());
    }

    hstring ConfigurationSetParser_0_1::GetSchemaVersion()
    {
        static hstring s_schemaVersion{ L"0.1" };
        return s_schemaVersion;
    }

    void ConfigurationSetParser_0_1::SetDocument(AppInstaller::YAML::Node&& document)
    {
        m_document = std::move(document);
    }

    void ConfigurationSetParser_0_1::ParseConfigurationUnitsFromField(const Node& document, ConfigurationField field, ConfigurationUnitIntent intent, std::vector<Configuration::ConfigurationUnit>& result)
    {
        ParseSequence(document, field, false, Node::Type::Mapping, [&](const Node& item)
            {
                auto configurationUnit = make_self<ConfigurationUnit>();
                ParseConfigurationUnit(configurationUnit.get(), item, intent);
                result.emplace_back(*configurationUnit);
            });
    }

    void ConfigurationSetParser_0_1::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode, ConfigurationUnitIntent intent)
    {
        CHECK_ERROR(GetStringValueForUnit(unitNode, ConfigurationField::Resource, true, unit, &ConfigurationUnit::Type));
        CHECK_ERROR(GetStringValueForUnit(unitNode, ConfigurationField::Id, false, unit, &ConfigurationUnit::Identifier));
        unit->Intent(intent);
        CHECK_ERROR(GetStringArrayForUnit(unitNode, ConfigurationField::DependsOn, false, unit, &ConfigurationUnit::Dependencies));
        CHECK_ERROR(ParseValueSet(unitNode, ConfigurationField::Directives, false, unit->Metadata()));
        CHECK_ERROR(ParseValueSet(unitNode, ConfigurationField::Settings, false, unit->Settings()));
    }
}
