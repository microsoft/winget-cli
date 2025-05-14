// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscAdminSettingsResource.h"
#include "DscComposableObject.h"
#include "Resources.h"
#include <winget/AdminSettings.h>

using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI
{
    namespace AdminSettingsDetails
    {
        // The base for admin settings.
        struct IAdminSetting
        {
            virtual ~IAdminSetting() = default;

            // Gets the name of the setting.
            virtual Utility::LocIndView SettingName() const = 0;

            // Tests the value.
            // Returns true if in the deired state; false if not.
            virtual bool Test() const = 0;

            // Sets the value.
            // Returns true if the value could be set; false if not.
            virtual bool Set() const = 0;
        };

        // A boolean based admin setting
        struct AdminSetting_Bool : public IAdminSetting
        {
            AdminSetting_Bool(Settings::BoolAdminSetting setting, bool value) : m_setting(setting), m_value(value) {}

            Utility::LocIndView SettingName() const override
            {
                return Settings::AdminSettingToString(m_setting);
            }

            bool Test() const override
            {
                return Settings::IsAdminSettingEnabled(m_setting) == m_value;
            }

            bool Set() const override
            {
                return m_value ? Settings::EnableAdminSetting(m_setting) : Settings::DisableAdminSetting(m_setting);
            }

        private:
            Settings::BoolAdminSetting m_setting;
            bool m_value;
        };

        // A string based admin setting
        struct AdminSetting_String : public IAdminSetting
        {
            AdminSetting_String(Settings::StringAdminSetting setting, std::optional<std::string> value) : m_setting(setting), m_value(std::move(value)) {}

            Utility::LocIndView SettingName() const override
            {
                return Settings::AdminSettingToString(m_setting);
            }

            bool Test() const override
            {
                return Settings::GetAdminSetting(m_setting) == m_value;
            }

            bool Set() const override
            {
                return m_value ? Settings::SetAdminSetting(m_setting, m_value.value()) : Settings::ResetAdminSetting(m_setting);
            }

        private:
            Settings::StringAdminSetting m_setting;
            std::optional<std::string> m_value;
        };
    }

    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(SettingsProperty, Json::Value, Settings, "settings", Resource::String::DscResourcePropertyDescriptionAdminSettingsSettings);

        using AdminSettingsResourceObject = DscComposableObject<StandardInDesiredStateProperty, SettingsProperty>;

        struct AdminSettingsFunctionData
        {
            AdminSettingsFunctionData() = default;

            AdminSettingsFunctionData(const std::optional<Json::Value>& json) :
                Input(json)
            {
            }

            const AdminSettingsResourceObject Input;
            AdminSettingsResourceObject Output;
            std::vector<std::unique_ptr<AdminSettingsDetails::IAdminSetting>> InputSettings;

            // Converts the input settings into the appropriate settings object.
            void ParseSettings()
            {
                if (Input.Settings())
                {
                    const Json::Value& inputSettings = Input.Settings().value();
                    for (const auto& property : inputSettings.getMemberNames())
                    {
                        auto boolSetting = Settings::StringToBoolAdminSetting(property);
                        if (boolSetting != Settings::BoolAdminSetting::Unknown)
                        {
                            bool value = inputSettings[property].asBool();
                            AICLI_LOG(Config, Info, << "Bool admin setting: " << property << " => " << (value ? "true" : "false"));
                            InputSettings.emplace_back(std::make_unique<AdminSettingsDetails::AdminSetting_Bool>(boolSetting, value));
                            continue;
                        }

                        auto stringSetting = Settings::StringToStringAdminSetting(property);
                        if (stringSetting != Settings::StringAdminSetting::Unknown)
                        {
                            std::string value = inputSettings[property].asString();
                            AICLI_LOG(Config, Info, << "String admin setting: " << property << " => " << value);
                            InputSettings.emplace_back(std::make_unique<AdminSettingsDetails::AdminSetting_String>(stringSetting, value));
                            continue;
                        }

                        AICLI_LOG(Config, Warning, << "Unknown admin setting: " << property);
                    }
                }
            }

            // Fills the Output object with the current state
            void Get()
            {
                Json::Value adminSettings{ Json::objectValue };

                for (const auto& setting : Settings::GetAllBoolAdminSettings())
                {
                    auto str = std::string{ Settings::AdminSettingToString(setting) };
                    adminSettings[str] = Settings::IsAdminSettingEnabled(setting);
                }

                for (const auto& setting : Settings::GetAllStringAdminSettings())
                {
                    auto name = std::string{ Settings::AdminSettingToString(setting) };
                    auto value = Settings::GetAdminSetting(setting);
                    if (value)
                    {
                        adminSettings[name] = value.value();
                    }
                }

                Output.Settings(std::move(adminSettings));
            }

            // Determines if the current Output values match the Input values state.
            bool Test()
            {
                for (const auto& setting : InputSettings)
                {
                    if (!setting->Test())
                    {
                        return false;
                    }
                }

                return true;
            }

            // Sets all of the input settings.
            void Set()
            {
                for (const auto& setting : InputSettings)
                {
                    if (!setting->Test())
                    {
                        if (!setting->Set())
                        {
                            auto message = Resource::String::DisabledByGroupPolicy(setting->SettingName());
                            THROW_HR_MSG(APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY, "%hs", message.get().c_str());
                        }
                    }
                }
            }

            Json::Value DiffJson(bool inDesiredState)
            {
                Json::Value result{ Json::ValueType::arrayValue };

                if (!inDesiredState)
                {
                    result.append(std::string{ SettingsProperty::Name() });
                }

                return result;
            }
        };
    }

    DscAdminSettingsResource::DscAdminSettingsResource(std::string_view parent) :
        DscCommandBase(parent, "admin-settings", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::ReturnsStateAndDiff)
    {
    }

    Resource::LocString DscAdminSettingsResource::ShortDescription() const
    {
        return Resource::String::DscAdminSettingsResourceShortDescription;
    }

    Resource::LocString DscAdminSettingsResource::LongDescription() const
    {
        return Resource::String::DscAdminSettingsResourceLongDescription;
    }

    std::string DscAdminSettingsResource::ResourceType() const
    {
        return "AdminSettings";
    }

    void DscAdminSettingsResource::ResourceFunctionGet(Execution::Context& context) const
    {
        AdminSettingsFunctionData data;
        data.Get();
        WriteJsonOutputLine(context, data.Output.ToJson());
    }

    void DscAdminSettingsResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            AdminSettingsFunctionData data{ json };

            data.ParseSettings();

            bool inDesiredState = data.Test();
            if (!inDesiredState)
            {
                Workflow::EnsureRunningAsAdmin(context);

                if (context.IsTerminated())
                {
                    return;
                }

                data.Set();
            }

            data.Get();
            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, data.DiffJson(inDesiredState));
        }
    }

    void DscAdminSettingsResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            AdminSettingsFunctionData data{ json };

            data.ParseSettings();

            data.Get();
            data.Output.InDesiredState(data.Test());

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, data.DiffJson(data.Output.InDesiredState().value()));
        }
    }

    void DscAdminSettingsResource::ResourceFunctionExport(Execution::Context& context) const
    {
        ResourceFunctionGet(context);
    }

    void DscAdminSettingsResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, AdminSettingsResourceObject::Schema(ResourceType()));
    }
}
