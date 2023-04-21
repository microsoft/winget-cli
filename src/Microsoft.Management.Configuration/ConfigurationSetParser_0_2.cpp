// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetParser_0_2.h"

#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

#include <sstream>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;

#define FIELD_TYPE_ERROR(_field_) SetError(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, (_field_)); return
#define FIELD_TYPE_ERROR_IF(_condition_,_field_) if (_condition_) { FIELD_TYPE_ERROR(_field_); }
#define FIELD_VALUE_ERROR(_field_,_value_) SetError(WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, (_field_), (_value_)); return
#define FIELD_VALUE_ERROR_IF(_condition_,_field_,_value_) if (_condition_) { FIELD_VALUE_ERROR(_field_,_value_); }

    namespace
    {
        // Contains the qualified resource name information.
        struct QualifiedResourceName
        {
            QualifiedResourceName(hstring input)
            {
                std::wstring_view inputView = input;
                size_t pos = inputView.find('\\');

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

    hstring ConfigurationSetParser_0_2::GetSchemaVersion()
    {
        static hstring s_schemaVersion{ L"0.2" };
        return s_schemaVersion;
    }

    void ConfigurationSetParser_0_2::ParseConfigurationUnit(ConfigurationUnit* unit, const Node& unitNode, ConfigurationUnitIntent intent)
    {
        using namespace AppInstaller::Utility;

        ConfigurationSetParser_0_1::ParseConfigurationUnit(unit, unitNode, intent);

        // Move module qualification into directives if present
        QualifiedResourceName qualifiedName{ unit->UnitName() };

        FIELD_VALUE_ERROR_IF(qualifiedName.Resource.empty(), GetFieldName(FieldName::Resource), ConvertToUTF8(unit->UnitName()));

        if (!qualifiedName.Module.empty())
        {
            // If the module is provided in both the resource name and the directives, ensure that it matches
            hstring moduleDirectiveFieldName = GetFieldNameHString(FieldName::ModuleDirective);
            auto moduleDirective = unit->Directives().TryLookup(moduleDirectiveFieldName);
            if (moduleDirective)
            {
                auto moduleProperty = moduleDirective.try_as<Windows::Foundation::IPropertyValue>();
                FIELD_TYPE_ERROR_IF(!moduleProperty, GetFieldName(FieldName::ModuleDirective));
                FIELD_TYPE_ERROR_IF(moduleProperty.Type() != Windows::Foundation::PropertyType::String, GetFieldName(FieldName::ModuleDirective));
                hstring moduleValue = moduleProperty.GetString();
                FIELD_VALUE_ERROR_IF(qualifiedName.Module != moduleValue, GetFieldName(FieldName::ModuleDirective), ConvertToUTF8(moduleValue));
            }
            else
            {
                unit->Directives().Insert(moduleDirectiveFieldName, Windows::Foundation::PropertyValue::CreateString(qualifiedName.Module));
            }

            // Set the unit name to be just the resource portion
            unit->UnitName(qualifiedName.Resource);
        }
    }
}
