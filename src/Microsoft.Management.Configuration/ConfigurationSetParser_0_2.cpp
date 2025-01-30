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

    void ConfigurationSetParser_0_2::SetDocument(AppInstaller::YAML::Node&& document)
    {
        m_document = std::move(document);
    }

    void ConfigurationSetParser_0_2::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode, ConfigurationUnitIntent intent)
    {
        CHECK_ERROR(ConfigurationSetParser_0_1::ParseConfigurationUnit(unit, unitNode, intent));
        ValidateType(unit, unitNode, ConfigurationField::Resource, true, false);
        ExtractSecurityContext(unit);
    }
}
