// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscTestFileResource.h"
#include "DscComposableObject.h"

using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    namespace anon
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(PathProperty, std::string, Path, "path", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, "The absolute path to a file.");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(ContentProperty, std::string, Content, "content", "The content of the file.");

        using TestFileObject = DscComposableObject<StandardExistProperty, StandardInDesiredStateProperty, PathProperty, ContentProperty>;

        struct TestFileFunctionData
        {
            TestFileFunctionData(const std::optional<Json::Value>& json) : Input(json), Output(Input.CopyForOutput())
            {
                Path = Utility::ConvertToUTF16(Input.Path().value());
                THROW_HR_IF(E_INVALIDARG, !Path.is_absolute());
            }

            TestFileObject Input;
            TestFileObject Output;
            std::filesystem::path Path;

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

    DscTestFileResource::DscTestFileResource(std::string_view parent) :
        DscCommandBase(parent, "test-file", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::HandlesExist | DscFunctionModifiers::ReturnsStateAndDiff)
    {
    }

    Resource::LocString DscTestFileResource::ShortDescription() const
    {
        return "[TEST] File content resource"_lis;
    }

    Resource::LocString DscTestFileResource::LongDescription() const
    {
        return "[TEST] This resource is only available for tests. It provides file content configuration."_lis;
    }

    std::string DscTestFileResource::ResourceType() const
    {
        return "TestFile";
    }

    void DscTestFileResource::ResourceFunctionGet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestFileFunctionData data{ json };

            data.Get();

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscTestFileResource::ResourceFunctionSet(Execution::Context& context) const
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

    void DscTestFileResource::ResourceFunctionTest(Execution::Context& context) const
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

    void DscTestFileResource::ResourceFunctionExport(Execution::Context& context) const
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

    void DscTestFileResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, anon::TestFileObject::Schema(ResourceType()));
    }
}
