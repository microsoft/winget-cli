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
        WINGET_DSC_DEFINE_COMPOSABLE_REQUIRED_PROPERTY(PathProperty, std::string, Path, "path");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(ContentProperty, std::string, Content, "content");

        using TestFileObject = DscComposableObject<StandardExistProperty, PathProperty, ContentProperty>;
    }

    DscTestFileResource::DscTestFileResource(std::string_view parent) :
        DscCommandBase(parent, "test-file", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::WhatIf | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::HandlesExist)
    {
    }

    Resource::LocString DscTestFileResource::ShortDescription() const
    {
        return "[TEST] File content resource"_lis;
    }

    Resource::LocString DscTestFileResource::LongDescription() const
    {
        return "This resource is only available for tests. It provides file content configuration."_lis;
    }

    std::string DscTestFileResource::ResourceType() const
    {
        return "TestFile";
    }

    void DscTestFileResource::ResourceFunctionGet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            anon::TestFileObject input{ json };

            std::filesystem::path path{ Utility::ConvertToUTF16(input.Path().value()) };
            if (std::filesystem::exists(path))
            {
                input.Exist(true);

                std::ifstream stream(path);
                input.Content(Utility::ReadEntireStream(stream));
            }
            else
            {
                input.Exist(false);
            }

            WriteJsonOutput(context, input.ToJson());
        }
    }

    void DscTestFileResource::ResourceFunctionSet(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
        THROW_HR(E_NOTIMPL);
    }

    void DscTestFileResource::ResourceFunctionWhatIf(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
        THROW_HR(E_NOTIMPL);
    }

    void DscTestFileResource::ResourceFunctionTest(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
        THROW_HR(E_NOTIMPL);
    }

    void DscTestFileResource::ResourceFunctionExport(Execution::Context& context) const
    {
        UNREFERENCED_PARAMETER(context);
        THROW_HR(E_NOTIMPL);
    }

    void DscTestFileResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutput(context, anon::TestFileObject::Schema());
    }
}
