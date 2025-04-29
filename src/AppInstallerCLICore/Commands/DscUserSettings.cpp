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

        // Merges the overlay settings into the target settings.
        void MergeUserSettings(Json::Value& target, const Json::Value& overlay)
        {
            // If either is not an object, we can't merge.
            if (!overlay.isObject() || !target.isObject())
            {
                return;
            }

            // Iterate through the overlay settings and merge them into the target.
            for (const auto& overlayKey : overlay.getMemberNames())
            {
                const Json::Value& overlayValue = overlay[overlayKey];
                if (target.isMember(overlayKey))
                {
                    Json::Value& targetValue = target[overlayKey];
                    if (targetValue.isObject() && overlayValue.isObject())
                    {
                        // Recursively merge the objects.
                        MergeUserSettings(targetValue, overlayValue);
                    }
                    else
                    {
                        // Replace the value in the target.
                        // Note: Arrays are not merged, they are replaced.
                        target[overlayKey] = overlayValue;
                    }
                }
                else
                {
                    // Add the overlay key to the target.
                    target[overlayKey] = overlayValue;
                }
            }
        }

        // Reads the user settings from the settings file.
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

        // Writes the user settings to the settings file.
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

        // Processes the user settings by merging them with the existing settings.
        bool TryProcessUserSettings(const UserSettingsObject& input, UserSettingsObject& output, Json::Value userSettings = Json::nullValue)
        {
            // Always return the full settings object
            output.Action(ACTION_FULL);

            // Full: overwrite the settings file
            if(input.Action() == ACTION_FULL)
            {
                output.Settings(input.Settings());
                return true;
            }

            // Partial: merge the settings
            if (userSettings || TryReadUserSettings(userSettings))
            {
                MergeUserSettings(userSettings, *input.Settings());
                output.Settings(userSettings);
                return true;
            }

            return false;
        }

        // Exports the user settings to the output object.
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
            if (TryExportUserSettings(outputExport) && TryProcessUserSettings(input, outputProcess, *outputExport.Settings()))
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
