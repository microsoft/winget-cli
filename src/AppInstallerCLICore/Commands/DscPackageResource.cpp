// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscPackageResource.h"
#include "DscComposableObject.h"
#include "Resources.h"
#include "Workflows/WorkflowBase.h"

using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI
{
    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(IdProperty, std::string, Identifier, "id", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageId);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(SourceProperty, std::string, Source, "source", DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageSource);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(VersionProperty, std::string, Version, "version", Resource::String::DscResourcePropertyDescriptionPackageVersion);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM_FLAGS(ScopeProperty, std::string, Scope, "scope", DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageScope, ({ "user", "machine" }), {});
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM_FLAGS(MatchOptionProperty, std::string, MatchOption, "matchOption", DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageMatchOption, ({ "equals", "equalsCaseInsensitive", "startsWithCaseInsensitive", "containsCaseInsensitive" }), "equalsCaseInsensitive");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_DEFAULT(UseLatestProperty, bool, UseLatest, "useLatest", Resource::String::DscResourcePropertyDescriptionPackageUseLatest, "false");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(InstallModeProperty, std::string, InstallMode, "installMode", Resource::String::DscResourcePropertyDescriptionPackageInstallMode, ({ "default", "silent", "interactive" }), "silent");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(AcceptAgreementsProperty, bool, AcceptAgreements, "acceptAgreements", Resource::String::DscResourcePropertyDescriptionPackageAcceptAgreements);

        using PackageResourceObject = DscComposableObject<StandardExistProperty, StandardInDesiredStateProperty, IdProperty, SourceProperty, VersionProperty, ScopeProperty, MatchOptionProperty, UseLatestProperty, InstallModeProperty, AcceptAgreementsProperty>;

        std::optional<MatchType> ToMatchType(const std::optional<std::string>& value)
        {
            if (!value)
            {
                return std::nullopt;
            }

            std::string lowerValue = Utility::ToLower(value.value());

            if (lowerValue == "equals")
            {
                return MatchType::Exact;
            }
            else if (lowerValue == "equalscaseinsensitive")
            {
                return MatchType::CaseInsensitive;
            }
            else if (lowerValue == "startswithcaseinsensitive")
            {
                return MatchType::StartsWith;
            }
            else if (lowerValue == "containscaseinsensitive")
            {
                return MatchType::Substring;
            }

            THROW_HR(E_INVALIDARG);
        }

        struct PackageFunctionData
        {
            PackageFunctionData(Execution::Context& context, const std::optional<Json::Value>& json) :
                Input(json),
                Output(Input.CopyForOutput()),
                ParentContext(context),
                SubContext(context.CreateSubContext())
            {
                SubContext->SetFlags(Execution::ContextFlag::DisableInteractivity);

                if (Input.AcceptAgreements().value_or(false))
                {
                    SubContext->Args.AddArg(Execution::Args::Type::AcceptSourceAgreements);
                    SubContext->Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);
                }
            }

            PackageResourceObject Input;
            PackageResourceObject Output;
            Execution::Context& ParentContext;
            std::unique_ptr<Execution::Context> SubContext;

            // Fills the Output object with the current state
            void Get()
            {
                if (Input.Source())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::Source, Input.Source().value());
                }

                if (Input.Scope() && !Input.Scope().value().empty())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::InstallScope, Input.Scope().value());
                }

                *SubContext <<
                    Workflow::OpenSource() <<
                    Workflow::OpenCompositeSource(Workflow::DetermineInstalledSource(*SubContext));

                if (SubContext->IsTerminated())
                {
                    ParentContext.Terminate(SubContext->GetTerminationHR());
                    return;
                }

                // Do a manual search of the now opened source
                Source& source = SubContext->Get<Execution::Data::Source>();
                MatchType matchType = ToMatchType(Input.MatchOption()).value_or(MatchType::CaseInsensitive);

                SearchRequest request;
                request.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, matchType, Input.Identifier().value()));

                SearchResult result = source.Search(request);

                if (result.Matches.empty())
                {
                    Output.Exist(false);
                }
                else if (result.Matches.size() > 1)
                {
                    AICLI_LOG(Config, Warning, << "Found " << result.Matches.size() << " matches when searching for '" << Input.Identifier().value() << "'");
                    Output.Exist(false);
                }
                else
                {
                    auto& package = result.Matches.front().Package;
                    auto installedPackage = package->GetInstalled();
                    THROW_HR_IF(E_UNEXPECTED, !installedPackage);
                    auto installedVersion = installedPackage->GetLatestVersion();
                    THROW_HR_IF(E_UNEXPECTED, !installedVersion);
                    auto metadata = installedVersion->GetMetadata();

                    // Fill Output and SubContext
                    Output.Exist(true);
                    Output.Identifier(package->GetProperty(PackageProperty::Id));
                    Output.Version(installedVersion->GetProperty(PackageVersionProperty::Version));

                    auto scopeItr = metadata.find(PackageVersionMetadata::InstalledScope);
                    if (scopeItr != metadata.end())
                    {
                        Output.Scope(scopeItr->second);
                    }
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

        private:
            bool TestVersion()
            {
                if (Input.Version())
                {
                    if (Output.Version())
                    {
                        return Utility::Version{ Input.Version().value() } == Utility::Version{ Output.Version().value() };
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return true;
                }
            }

            bool TestScope()
            {
                if (Input.Scope())
                {
                    if (Output.Scope())
                    {
                        return Utility::ToLower(Input.Scope().value()) == Utility::ToLower(Output.Scope().value());
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return true;
                }
            }

            bool TestLatest()
            {
                if (Input.UseLatest())
                {

                }
                else
                {
                    return true;
                }
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
            PackageFunctionData data{ context, json };

            data.Get();

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscPackageResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            PackageFunctionData data{ context, json };

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
            PackageFunctionData data{ context, json };

            THROW_HR(E_NOTIMPL);
        }
    }

    void DscPackageResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            PackageFunctionData data{ context, json };

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
            PackageFunctionData data{ context, json };

            THROW_HR(E_NOTIMPL);
        }
    }

    void DscPackageResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, PackageResourceObject::Schema(ResourceType()));
    }
}
