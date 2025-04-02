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
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_DEFAULT(UseLatestProperty, bool, UseLatest, "useLatest", "Indicate that the latest available version of the package should be installed.", "false");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(InstallModeProperty, std::string, InstallMode, "installMode", "The install mode to use if needed.", ({ "default", "silent", "interactive" }), "silent");

        using PackageResourceObject = DscComposableObject<StandardExistProperty, StandardInDesiredStateProperty, IdProperty, SourceProperty, VersionProperty, MatchOptionProperty, UseLatestProperty, InstallModeProperty>;

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
                THROW_HR(E_NOTIMPL);
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
                        THROW_HR(E_NOTIMPL);
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
                    THROW_HR(E_NOTIMPL);
                }

                return result;
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
            PackageFunctionData data{ json };

            data.Get();

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscPackageResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            PackageFunctionData data{ json };

            data.Get();

            if (!data.Test())
            {
                THROW_HR(E_NOTIMPL);
            }

            // Capture the diff before updating the output
            auto diff = data.DiffJson();

            data.Output.Exist(data.Input.ShouldExist());
            if (data.Output.Exist().value())
            {
                THROW_HR(E_NOTIMPL);
            }

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, diff);
        }
    }

    void DscPackageResource::ResourceFunctionWhatIf(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            PackageFunctionData data{ json };

            THROW_HR(E_NOTIMPL);
        }
    }

    void DscPackageResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            PackageFunctionData data{ json };

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
            PackageFunctionData data{ json };

            THROW_HR(E_NOTIMPL);
        }
    }

    void DscPackageResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, PackageResourceObject::Schema(ResourceType()));
    }
}
