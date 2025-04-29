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

        using UserSettingsObject = DscComposableObject<StandardExistProperty, StandardInDesiredStateProperty, SettingsProperty, ActionProperty>;

        bool TryReadUserSettings(Execution::Context& context, Json::Value& root)
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
                else
                {
                    context.Reporter.Error() << "Failed to parse settings file: " << settingsPath << ". Error: " << errs;
                }
            }
            else
            {
                context.Reporter.Error() << "Error opening settings file: " << settingsPath;
            }

            return false;
        }

        bool TryWriteUserSettings(Execution::Context& context, Json::Value& root)
        {
            auto settingsPath = UserSettings::SettingsFilePath();
            std::ofstream file(settingsPath, std::ios::binary);
            if (file)
            {
                Json::StreamWriterBuilder writer;
                file << Json::writeString(writer, root);
            }
            else
            {
                context.Reporter.Error() << "Error opening settings file for writing: " << settingsPath;
            }

            return true;
        }

        void MergeJson(Json::Value& target, const Json::Value& overlay)
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
                        MergeJson(targetValue, overlayValue);
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
        Json::Value root;
        if (TryReadUserSettings(context, root))
        {
            UserSettingsObject output;
            output.Action(ACTION_FULL);
            output.Settings(root);
            WriteJsonOutputLine(context, output.ToJson());
        }
    }

    void DscUserSettings::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            UserSettingsObject input(json);
            if (input.Settings())
            {
                UserSettingsObject output;
                output.Action(ACTION_FULL);

                // Full: overwrite the settings file
                // Partial: merge the settings file
                if (input.Action() == ACTION_PARTIAL)
                {
                    Json::Value root;
                    if (TryReadUserSettings(context, root))
                    {
                        MergeJson(root, *input.Settings());
                        output.Settings(root);
                    }
                    else
                    {
                        context.Reporter.Error() << "Failed to read settings file.";
                        return;
                    }
                }
                else
                {
                    output.Settings(input.Settings());
                }

                // Update the settings file
                if (TryWriteUserSettings(context, *output.Settings()))
                {
                    WriteJsonOutputLine(context, output.ToJson());
                }
                else
                {
                    context.Reporter.Error() << "Failed to write settings file.";
                }
            }
        }
    }

    void DscUserSettings::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            // TODO
        }
    }

    void DscUserSettings::ResourceFunctionExport(Execution::Context& context) const
    {
        auto json = GetJsonFromInput(context, false);
        // TODO
    }

    void DscUserSettings::ResourceFunctionSchema(Execution::Context& context) const
    {
        // TODO
        context;
    }
}
