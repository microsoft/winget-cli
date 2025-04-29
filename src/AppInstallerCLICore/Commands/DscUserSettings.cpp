// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscUserSettings.h"
#include "DscComposableObject.h"
#include "Resources.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/ConfigurationFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UninstallFlow.h"
#include "Workflows/UpdateFlow.h"
#include <winget/PackageVersionSelection.h>

using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Settings;

#define ACTION_FULL "Full"
#define ACTION_PARTIAL "Partial"

namespace AppInstaller::CLI
{
    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(SettingsProperty, Json::Value, Settings, "Settings", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageId);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(ActionProperty, std::string, Action, "Action", Resource::String::DscResourcePropertyDescriptionPackageMatchOption, ({ ACTION_PARTIAL, ACTION_FULL }), ACTION_FULL);

        using UserSettingsObject = DscComposableObject<StandardInDesiredStateProperty, SettingsProperty, ActionProperty>;

        void MergeUserSettings(Json::Value& target, const Json::Value& overlay)
        {
            if (!overlay.isObject() || !target.isObject())
            {
                return;
            }

            for (const auto& overlayKey : overlay.getMemberNames())
            {
                const Json::Value& overlayValue = overlay[overlayKey];
                if (target.isMember(overlayKey))
                {
                    Json::Value& targetValue = target[overlayKey];
                    if (targetValue.isObject() && overlayValue.isObject())
                    {
                        MergeUserSettings(targetValue, overlayValue);
                    }
                    else
                    {
                        target[overlayKey] = overlayValue;
                    }
                }
                else
                {
                    target[overlayKey] = overlayValue;
                }
            }
        }

        bool TryReadUserSettings(Json::Value& root)
        {
            auto settingsPath = UserSettings::SettingsFilePath();
            std::ifstream file(settingsPath, std::ios::binary);
            if (file)
            {
                Json::CharReaderBuilder builder;
                std::string errs;
                if (Json::parseFromStream(builder, file, &root, &errs))
                {
                    return true;
                }
            }

            return false;
        }

        bool TryWriteUserSettings(Json::Value& root)
        {
            auto settingsPath = UserSettings::SettingsFilePath();
            std::ofstream file(settingsPath, std::ios::binary);
            if (file)
            {
                Json::StreamWriterBuilder writer;
                file << Json::writeString(writer, root);
            }

            return true;
        }

        bool TryProcessUserSettings(const UserSettingsObject& input, UserSettingsObject& output)
        {
            // Input settings property is required
            if (!input.Settings())
            {
                return false;
            }

            // Always return the full settings object
            output.Action(ACTION_FULL);

            // Full: overwrite the settings file
            if(input.Action() == ACTION_FULL)
            {
                output.Settings(input.Settings());
                return true;
            }

            // Partial: merge the settings
            Json::Value userSettings;
            if (TryReadUserSettings(userSettings))
            {
                MergeUserSettings(userSettings, *input.Settings());
                output.Settings(userSettings);
                return true;
            }

            return false;
        }

        bool TryExportUserSettings(UserSettingsObject& output)
        {
            Json::Value userSettings;
            if (TryReadUserSettings(userSettings))
            {
                output.Action(ACTION_FULL);
                output.Settings(userSettings);
                return true;
            }

            return false;
        }
    }

    DscUserSettings::DscUserSettings(std::string_view parent) :
        DscCommandBase(parent, "user-settings", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::HandlesExist | DscFunctionModifiers::ReturnsStateAndDiff)
    {
    }

    Resource::LocString DscUserSettings::ShortDescription() const
    {
        return Resource::String::DscPackageResourceShortDescription;
    }

    Resource::LocString DscUserSettings::LongDescription() const
    {
        return Resource::String::DscPackageResourceLongDescription;
    }

    std::string DscUserSettings::ResourceType() const
    {
        return "WinGetUserSettings";
    }

    void DscUserSettings::ResourceFunctionGet(Execution::Context& context) const
    {
        UserSettingsObject output;
        if (TryExportUserSettings(output))
        {
            WriteJsonOutputLine(context, output.ToJson());
        }
    }

    void DscUserSettings::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            UserSettingsObject input(json);
            UserSettingsObject output;
            if (TryProcessUserSettings(input, output) && TryWriteUserSettings(*output.Settings()))
            {
                WriteJsonOutputLine(context, output.ToJson());
            }
        }
    }

    void DscUserSettings::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            UserSettingsObject input(json);
            UserSettingsObject outputProcess;
            UserSettingsObject outputExport;
            if (TryProcessUserSettings(input, outputProcess) && TryExportUserSettings(outputExport))
            {
                outputExport.InDesiredState(outputProcess.Settings() == outputExport.Settings());
                WriteJsonOutputLine(context, outputExport.ToJson());
            }
        }
    }

    void DscUserSettings::ResourceFunctionExport(Execution::Context& context) const
    {
        UserSettingsObject output;
        if (TryExportUserSettings(output))
        {
            WriteJsonOutputLine(context, output.ToJson());
        }
    }

    void DscUserSettings::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, UserSettingsObject::Schema(ResourceType()));
    }
}
