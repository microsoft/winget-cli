// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscUserSettingsFileResource.h"
#include "DscComposableObject.h"
#include "Resources.h"
#include "AppInstallerStrings.h"

using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Settings;

#define ACTION_FULL "Full"
#define ACTION_PARTIAL "Partial"

namespace AppInstaller::CLI
{
    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(SettingsProperty, Json::Value, Settings, "settings", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionUserSettingsFileSettings);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(ActionProperty, std::string, Action, "action", Resource::String::DscResourcePropertyDescriptionUserSettingsFileAction, ({ ACTION_PARTIAL, ACTION_FULL }), ACTION_PARTIAL);

        using UserSettingsFileResourceObject = DscComposableObject<StandardInDesiredStateProperty, SettingsProperty, ActionProperty>;

        struct UserSettingsFileFunctionData
        {
            UserSettingsFileFunctionData()
                : UserSettingsFileFunctionData(std::nullopt, true)
            {
            }

            UserSettingsFileFunctionData(const std::optional<Json::Value>& json, bool ignoreFieldRequirements = false) :
                Input(json, ignoreFieldRequirements),
                _userSettingsPath(UserSettings::SettingsFilePath())
            {
                const auto& action = Input.Action();
                if (action.has_value() && Utility::CaseInsensitiveEquals(action.value(), ACTION_FULL))
                {
                    Output.Action(ACTION_FULL);
                }
                else
                {
                    Output.Action(ACTION_PARTIAL);
                }
            }

            const UserSettingsFileResourceObject Input;
            UserSettingsFileResourceObject Output;

            void Get()
            {
                Output.Settings(GetUserSettings());
            }

            bool Test()
            {
                return GetResolvedInput() == Output.Settings();
            }

            Json::Value DiffJson()
            {
                Json::Value result{ Json::ValueType::arrayValue };

                if (!Test())
                {
                    result.append(std::string{ SettingsProperty::Name() });
                }

                return result;
            }

            const Json::Value& GetResolvedInput()
            {
                THROW_HR_IF(E_UNEXPECTED, !Input.Settings().has_value());
                if (!_resolvedInputUserSettings)
                {
                    if(Input.Action().has_value() && Utility::CaseInsensitiveEquals(Input.Action().value(), ACTION_FULL))
                    {
                        _resolvedInputUserSettings = Input.Settings();
                    }
                    else
                    {
                        _resolvedInputUserSettings = MergeUserSettingsFiles(*Input.Settings());
                    }
                }

                return *_resolvedInputUserSettings;
            }

            bool WriteOutput()
            {
                THROW_HR_IF(E_UNEXPECTED, !Output.Settings().has_value());
                std::ofstream file(_userSettingsPath, std::ios::binary);
                if (file)
                {
                    Json::StreamWriterBuilder writer;
                    writer["indentation"] = "  ";
                    file << Json::writeString(writer, *Output.Settings());
                    return true;
                }

                AICLI_LOG(Config, Error, << "Failed to open or create user settings file: " << _userSettingsPath);
                return false;
            }

        private:
            std::filesystem::path _userSettingsPath;
            std::optional<Json::Value> _userSettings;
            std::optional<Json::Value> _resolvedInputUserSettings;

            Json::Value MergeUserSettingsFiles(const Json::Value& overlay)
            {
                Json::Value mergedUserSettingsFile = GetUserSettings();
                MergeUserSettingsFiles(mergedUserSettingsFile, overlay);
                return mergedUserSettingsFile;
            }

            // Merges the overlay settings into the target settings.
            void MergeUserSettingsFiles(Json::Value& target, const Json::Value& overlay)
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
                            MergeUserSettingsFiles(targetValue, overlayValue);
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

            const Json::Value& GetUserSettings()
            {
                if (!_userSettings)
                {
                    _userSettings = Json::objectValue;
                    std::ifstream file(_userSettingsPath, std::ios::binary);
                    if (file)
                    {
                        Json::CharReaderBuilder builder;
                        std::string errs;
                        Json::Value jsonRoot;
                        if (Json::parseFromStream(builder, file, &jsonRoot, &errs))
                        {
                            _userSettings = jsonRoot;
                        }
                        else
                        {
                            AICLI_LOG(Config, Warning, << "Failed to parse user settings file: " << _userSettingsPath << ", error: " << errs);
                        }
                    }
                    else
                    {
                        AICLI_LOG(Config, Warning, << "Failed to open user settings file: " << _userSettingsPath);
                    }
                }

                return *_userSettings;
            }
        };
    }

    DscUserSettingsFileResource::DscUserSettingsFileResource(std::string_view parent) :
        DscCommandBase(parent, "user-settings-file", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::HandlesExist | DscFunctionModifiers::ReturnsStateAndDiff)
    {
    }

    Resource::LocString DscUserSettingsFileResource::ShortDescription() const
    {
        return Resource::String::DscUserSettingsFileShortDescription;
    }

    Resource::LocString DscUserSettingsFileResource::LongDescription() const
    {
        return Resource::String::DscUserSettingsFileLongDescription;
    }

    std::string DscUserSettingsFileResource::ResourceType() const
    {
        return "UserSettingsFile";
    }

    void DscUserSettingsFileResource::ResourceFunctionGet(Execution::Context& context) const
    {
        ResourceFunctionExport(context);
    }

    void DscUserSettingsFileResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            UserSettingsFileFunctionData data{ json };

            data.Get();

            // Capture the diff before updating the output
            auto diff = data.DiffJson();

            if (!data.Test())
            {
                data.Output.Settings(data.GetResolvedInput());
                if (!data.WriteOutput())
                {
                    AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_OPEN_FAILED));
                    return;
                }
            }

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, diff);
        }
    }

    void DscUserSettingsFileResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            UserSettingsFileFunctionData data{ json };

            data.Get();
            data.Output.InDesiredState(data.Test());

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, data.DiffJson());
        }
    }

    void DscUserSettingsFileResource::ResourceFunctionExport(Execution::Context& context) const
    {
        UserSettingsFileFunctionData data;

        data.Get();

        WriteJsonOutputLine(context, data.Output.ToJson());
    }

    void DscUserSettingsFileResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, UserSettingsFileResourceObject::Schema(ResourceType()));
    }
}
