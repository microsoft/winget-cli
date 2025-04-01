// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscPackageResource.h"
#include "DscComposableObject.h"
#include "Resources.h"

using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(IdProperty, std::string, Identifier, "id", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, "The identifier of the package.");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(SourceProperty, std::string, Source, "source", DscComposablePropertyFlag::CopyToOutput, "The source of the package.");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(VersionProperty, std::string, Version, "version", "The version of the package.");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(MatchOptionProperty, std::string, MatchOption, "matchOption", "The method for matching the identifier with a package.", ({ "equals", "equalsCaseInsensitive", "startsWithCaseInsensitive", "containsCaseInsensitive" }), "equalsCaseInsensitive");

        using PackageResourceObject = DscComposableObject<StandardExistProperty, StandardInDesiredStateProperty, IdProperty, SourceProperty>;

        struct PackageFunctionData
        {
            PackageFunctionData(const std::optional<Json::Value>& json) : Input(json), Output(Input.CopyForOutput())
            {
            }

            PackageResourceObject Input;
            PackageResourceObject Output;

            // Fills the Output object with the current state
            void Get()
            {
                if (std::filesystem::exists(Path) && std::filesystem::is_regular_file(Path))
                {
                    Output.Exist(true);

                    std::ifstream stream{ Path, std::ios::binary };
                    Output.Content(Utility::ReadEntireStream(stream));
                }
                else
                {
                    Output.Exist(false);
                }
            }

            // Determines if the current Output values match the Input values state.
            bool Test()
            {
                // Need to populate Output before calling
                THROW_HR_IF(E_UNEXPECTED, !Output.Exist().has_value());

                if (Input.ShouldExist())
                {
                    if (Output.Exist().value())
                    {
                        return ContentMatches();
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return !Output.Exist().value();
                }
            }

            Json::Value DiffJson()
            {
                // Need to populate Output before calling
                THROW_HR_IF(E_UNEXPECTED, !Output.Exist().has_value());

                Json::Value result{ Json::ValueType::arrayValue };

                if (Input.ShouldExist() != Output.Exist().value())
                {
                    result.append(std::string{ StandardExistProperty::Name() });
                }
                else
                {
                    if (!ContentMatches())
                    {
                        result.append(std::string{ ContentProperty::Name() });
                    }
                }

                return result;
            }

        private:
            bool ContentMatches()
            {
                bool hasInput = Input.Content().has_value() && !Input.Content().value().empty();
                bool hasOutput = Output.Content().has_value() && !Output.Content().value().empty();

                return
                    (hasInput && hasOutput && Input.Content().value() == Output.Content().value()) ||
                    (!hasInput && !hasOutput);
            }
        };
    }

    DscPackageResource::DscPackageResource(std::string_view parent) :
        DscCommandBase(parent, "package", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::WhatIf | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::HandlesExist | DscFunctionModifiers::ReturnsStateAndDiff)
    {
    }

    Resource::LocString DscPackageResource::ShortDescription() const
    {
        return Resource::String::DscPackageResourceShortDescription;
    }

    Resource::LocString DscPackageResource::LongDescription() const
    {
        return Resource::String::DscPackageResourceLongDescription;
    }

    std::string DscPackageResource::ResourceType() const
    {
        return "Package";
    }

    void DscPackageResource::ResourceFunctionGet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestFileFunctionData data{ json };

            data.Get();

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscPackageResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestFileFunctionData data{ json };

            data.Get();

            if (!data.Test())
            {
                bool exists = std::filesystem::exists(data.Path);
                if (exists)
                {
                    // Don't delete a directory or other special files in this test resource
                    THROW_WIN32_IF(ERROR_DIRECTORY_NOT_SUPPORTED, !std::filesystem::is_regular_file(data.Path));
                }

                if (data.Input.ShouldExist())
                {
                    std::filesystem::create_directories(data.Path.parent_path());

                    std::ofstream stream{ data.Path, std::ios::binary | std::ios::trunc };
                    if (data.Input.Content())
                    {
                        stream.write(data.Input.Content().value().c_str(), data.Input.Content().value().length());
                    }
                }
                else if (exists)
                {
                    std::filesystem::remove(data.Path);
                }
            }

            // Capture the diff before updating the output
            auto diff = data.DiffJson();

            data.Output.Exist(data.Input.ShouldExist());
            if (data.Output.Exist().value())
            {
                data.Output.Content(data.Input.Content().value_or(""));
            }

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, diff);
        }
    }

    void DscPackageResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestFileFunctionData data{ json };

            data.Get();
            data.Output.InDesiredState(data.Test());

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, data.DiffJson());
        }
    }

    void DscPackageResource::ResourceFunctionExport(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestFileFunctionData data{ json };

            if (std::filesystem::exists(data.Path))
            {
                if (std::filesystem::is_regular_file(data.Path))
                {
                    data.Get();
                    WriteJsonOutputLine(context, data.Output.ToJson());
                }
                else if (std::filesystem::is_directory(data.Path))
                {
                    for (const auto& file : std::filesystem::directory_iterator{ data.Path })
                    {
                        if (std::filesystem::is_regular_file(file))
                        {
                            anon::TestFileObject output;
                            output.Path(file.path().u8string());

                            std::ifstream stream{ file.path(), std::ios::binary};
                            output.Content(Utility::ReadEntireStream(stream));

                            WriteJsonOutputLine(context, output.ToJson());
                        }
                    }
                }
            }
        }
    }

    void DscPackageResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, anon::TestFileObject::Schema(ResourceType()));
    }
}
