// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser_0_2.h"
#include "ParsingMacros.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

#include <sstream>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;

    hstring ConfigurationSetParser_0_2::GetSchemaVersion()
    {
        static hstring s_schemaVersion{ L"0.2" };
        return s_schemaVersion;
    }

    void ConfigurationSetParser_0_2::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode, ConfigurationUnitIntent intent)
    {
        CHECK_ERROR(ConfigurationSetParser_0_1::ParseConfigurationUnit(unit, unitNode, intent));
        ValidateType(unit, unitNode, FieldName::Resource, true, false);
    }

    hstring ConfigurationSetSerializer_0_2::Serialize(ConfigurationSet* configurationSet)
    {
        Emitter emitter;

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(FieldName::Properties);

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(FieldName::ConfigurationVersion) << Value << AppInstaller::Utility::ConvertToUTF8(configurationSet->SchemaVersion());
        emitter << Key << GetConfigurationFieldName(FieldName::Resources);
        emitter << BeginSeq;

        // First sort units into intents.


        for (const auto& unit : configurationSet->Units())
        {
            emitter << BeginMap;
            // TODO: change from using details.
            emitter << Key << GetConfigurationFieldName(FieldName::Resource) << Value << AppInstaller::Utility::ConvertToUTF8(unit.Type());

            // Directives
            auto metadata = unit.Metadata();
            emitter << Key << GetConfigurationFieldName(FieldName::Directives);
            WriteYamlValueSet(emitter, metadata);

            //// Settings
            auto settings = unit.Settings();
            emitter << Key << GetConfigurationFieldName(FieldName::Settings);
            WriteYamlValueSet(emitter, settings);

            emitter << EndMap;
        }

        emitter << EndSeq;
        emitter << EndMap;

        std::ofstream outFileStream(L"C:\\Users\\ryfu\\Downloads\\yamlResult.yml");
        emitter.Emit(outFileStream);
        outFileStream.close();

        return winrt::to_hstring(emitter.str());
    }
}
