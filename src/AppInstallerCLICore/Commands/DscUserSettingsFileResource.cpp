// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscUserSettingsFileResource.h"
#include "DscComposableObject.h"
#include "Resources.h"

using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Settings;

#define ACTION_FULL "Full"
#define ACTION_PARTIAL "Partial"

namespace AppInstaller::CLI
{
    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(SettingsProperty, Json::Value, Settings, "settings", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionUserSettingsFileSettings);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(ActionProperty, std::string, Action, "action", Resource::String::DscResourcePropertyDescriptionUserSettingsFileAction, ({ ACTION_PARTIAL, ACTION_FULL }), ACTION_FULL);

        using UserSettingsFileResourceObject = DscComposableObject<StandardInDesiredStateProperty, SettingsProperty, ActionProperty>;

        struct UserSettingsFileFunctionData
        {
            UserSettingsFileFunctionData()
                : UserSettingsFileFunctionData(std::nullopt)
            {
            }

            UserSettingsFileFunctionData(const std::optional<Json::Value>& json) :
                Input(json, false),
                _userSettings(Json::nullValue),
                _userSettingsPath(UserSettings::SettingsFilePath())
            {
            }

            const UserSettingsFileResourceObject Input;
            UserSettingsFileResourceObject Output;

            void Get()
            {
                THROW_HR_IF(E_UNEXPECTED, _userSettings.isNull());
                Output.Action(ACTION_FULL);
                Output.Settings(_userSettings);
            }

            void Set()
            {
                THROW_HR_IF(E_UNEXPECTED, _userSettings.isNull());
                Output.Action(ACTION_FULL);

                if(Input.Action() == ACTION_FULL)
                {
                    Output.Settings(Input.Settings());
                }
                else
                {
                    auto mergedUserSettingsFile = MergeUserSettingsFiles(*Input.Settings());
                    Output.Settings(mergedUserSettingsFile);
                }
            }

            bool Test()
            {
                THROW_HR_IF(E_UNEXPECTED, _userSettings.isNull());
                return _userSettings == Output.Settings();
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

            bool LoadUserSettings()
            {
                std::ifstream file(_userSettingsPath, std::ios::binary);
                if (file)
                {
                    Json::CharReaderBuilder builder;
                    std::string errs;
                    if (Json::parseFromStream(builder, file, &_userSettings, &errs))
                    {
                        return true;
                    }

                    AICLI_LOG(Config, Error, << "Failed to parse user settings file: " << _userSettingsPath << ", error: " << errs);
                }
                else
                {
                    AICLI_LOG(Config, Error, << "Failed to open user settings file: " << _userSettingsPath);
                }

                return false;
            }

            bool WriteOutput()
            {
                THROW_HR_IF(E_UNEXPECTED, !Output.Settings().has_value());
                std::ofstream file(_userSettingsPath, std::ios::binary);
                if (file)
                {
                    Json::StreamWriterBuilder writer;
                    file << Json::writeString(writer, *Output.Settings());
                    return true;
                }

                AICLI_LOG(Config, Error, << "Failed to open user settings file for writing: " << _userSettingsPath);
                return false;
            }

        private:
            std::filesystem::path _userSettingsPath;
            Json::Value _userSettings;

            Json::Value MergeUserSettingsFiles(const Json::Value& overlay)
            {
                Json::Value mergedUserSettingsFile = _userSettings;
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

            if (data.LoadUserSettings())
            {
                data.Set();
                if (!data.Test() && !data.WriteOutput())
                {
                    AICLI_LOG(Config, Error, << "Failed to write output to user settings file.");
                    return;
                }

                WriteJsonOutputLine(context, data.Output.ToJson());
                WriteJsonOutputLine(context, data.DiffJson());
            }
        }
    }

    void DscUserSettingsFileResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            UserSettingsFileFunctionData data{ json };

            if (data.LoadUserSettings())
            {
                data.Set();
                data.Output.InDesiredState(data.Test());

                // Get diff before updating the output
                auto diffJson = data.DiffJson();

                data.Get();

                WriteJsonOutputLine(context, data.Output.ToJson());
                WriteJsonOutputLine(context, diffJson);
            }
        }
    }

    void DscUserSettingsFileResource::ResourceFunctionExport(Execution::Context& context) const
    {
        UserSettingsFileFunctionData data;
        if (data.LoadUserSettings())
        {
            data.Get();
            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscUserSettingsFileResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, UserSettingsFileResourceObject::Schema(ResourceType()));
    }
}
