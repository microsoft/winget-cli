// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscTestJsonResource.h"
#include "DscComposableObject.h"
#include <AppInstallerRuntime.h>

using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    namespace anon
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(PropertyProperty, std::string, Property, "property", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, "The JSON property name.");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(ValueProperty, Json::Value, Value, "value", "The value for the JSON property.");

        using TestJsonObject = DscComposableObject<StandardExistProperty, PropertyProperty, ValueProperty>;

        struct TestJsonFunctionData
        {
            TestJsonFunctionData()
            {
                InitializeFileData();
            }

            TestJsonFunctionData(const std::optional<Json::Value>& json) : Input(json), Output(Input.CopyForOutput())
            {
                InitializeFileData();
            }

            TestJsonObject Input;
            TestJsonObject Output;
            std::filesystem::path FilePath;
            Json::Value RootValue;

            static std::filesystem::path GetFilePath()
            {
                std::filesystem::path result = Runtime::GetPathTo(Runtime::PathName::LocalState);
                result /= "test-json-file.json";
                return result;
            }

            // Fills the Output object with the current state
            void Get()
            {
                const std::string& propertyName = Input.Property().value();
                const Json::Value* propertyValue = RootValue.find(propertyName.data(), propertyName.data() + propertyName.length());

                if (propertyValue)
                {
                    Output.Exist(true);

                    Output.Value(*propertyValue);
                }
                else
                {
                    Output.Exist(false);
                }
            }

        private:
            void InitializeFileData()
            {
                FilePath = GetFilePath();
                RootValue = GetJsonFromFile();
            }

            Json::Value GetJsonFromFile() const
            {
                Json::Value result;
                Json::CharReaderBuilder builder;
                Json::String errors;

                std::ifstream stream{ FilePath, std::ios::binary };

                if (stream)
                {
                    if (!Json::parseFromStream(builder, stream, &result, &errors))
                    {
                        AICLI_LOG(CLI, Warning, << "Failed to read test JSON file: " << errors);
                        result = Json::Value{};
                    }
                }
                else
                {
                    AICLI_LOG(CLI, Warning, << "Couldn't open test JSON file: " << FilePath);
                }

                return result;
            }
        };
    }

    DscTestJsonResource::DscTestJsonResource(std::string_view parent) :
        DscCommandBase(parent, "test-json", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::HandlesExist | DscFunctionModifiers::ReturnsState)
    {
    }

    std::vector<Argument> DscTestJsonResource::GetArguments() const
    {
        auto result = DscCommandBase::GetArguments();
        result.emplace_back(Execution::Args::Type::DscResourceFunctionDelete, Resource::String::DscResourceFunctionDescriptionDelete, ArgumentType::Flag);
        return result;
    }

    Resource::LocString DscTestJsonResource::ShortDescription() const
    {
        return "[TEST] JSON content resource"_lis;
    }

    Resource::LocString DscTestJsonResource::LongDescription() const
    {
        return "[TEST] This resource is only available for tests. It provides JSON content configuration of a well known file."_lis;
    }

    void DscTestJsonResource::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::DscResourceFunctionDelete))
        {
            std::filesystem::remove_all(anon::TestJsonFunctionData::GetFilePath());
            return;
        }

        DscCommandBase::ExecuteInternal(context);
    }

    std::string DscTestJsonResource::ResourceType() const
    {
        return "TestJSON";
    }

    void DscTestJsonResource::ResourceFunctionGet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestJsonFunctionData data{ json };

            data.Get();

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscTestJsonResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestJsonFunctionData data{ json };

            data.Get();

            if (data.RootValue.isNull())
            {
                data.RootValue = Json::Value{ Json::objectValue };
            }

            if (data.Input.ShouldExist())
            {
                data.RootValue[data.Input.Property().value()] = data.Input.Value().value_or(Json::Value{ Json::nullValue });
                data.Output.Exist(true);
                data.Output.Value(data.RootValue[data.Input.Property().value()]);
            }
            else if (data.Output.Exist().value())
            {
                data.RootValue.removeMember(data.Input.Property().value());
                data.Output.Exist(false);
            }

            std::ofstream stream{ data.FilePath, std::ios::binary };

            Json::StreamWriterBuilder writerBuilder;
            writerBuilder.settings_["indentation"] = "  ";

            stream << Json::writeString(writerBuilder, data.RootValue);

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscTestJsonResource::ResourceFunctionExport(Execution::Context& context) const
    {
        anon::TestJsonFunctionData data;

        if (data.RootValue.isObject())
        {
            for (const auto& member : data.RootValue.getMemberNames())
            {
                const Json::Value* memberValue = data.RootValue.find(member.data(), member.data() + member.length());

                if (memberValue)
                {
                    anon::TestJsonObject output;
                    output.Property(member);
                    output.Value(*memberValue);

                    WriteJsonOutputLine(context, output.ToJson());
                }
            }
        }
    }

    void DscTestJsonResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, anon::TestJsonObject::Schema(ResourceType()));
    }
}
