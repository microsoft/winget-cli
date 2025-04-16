// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscPackageResource.h"
#include "DscComposableObject.h"
#include "Resources.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/ConfigurationFlow.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/UninstallFlow.h"
#include "Workflows/UpdateFlow.h"
#include <winget/PackageVersionSelection.h>

using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI
{
    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(IdProperty, std::string, Identifier, "id", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageId);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(SourceProperty, std::string, Source, "source", DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageSource);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(VersionProperty, std::string, Version, "version", Resource::String::DscResourcePropertyDescriptionPackageVersion);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM_FLAGS(ScopeProperty, std::string, Scope, "scope", DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionPackageScope, ({ "user", "system" }), {});
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
            else if (lowerValue == "equals""case""insensitive")
            {
                return MatchType::CaseInsensitive;
            }
            else if (lowerValue == "starts""with""case""insensitive")
            {
                return MatchType::StartsWith;
            }
            else if (lowerValue == "contains""case""insensitive")
            {
                return MatchType::Substring;
            }

            THROW_HR(E_INVALIDARG);
        }

        std::string ConvertScope(std::string_view value, bool preferSystem)
        {
            std::string lowerValue = Utility::ToLower(value);

            if (lowerValue == "machine" || lowerValue == "system")
            {
                return preferSystem ? "system" : "machine";
            }
            else
            {
                return std::string{ value };
            }
        }

        struct PackageFunctionData
        {
            PackageFunctionData(Execution::Context& context, const std::optional<Json::Value>& json, bool ignoreFieldRequirements = false) :
                Input(json, ignoreFieldRequirements),
                ParentContext(context)
            {
                Reset();
            }

            const PackageResourceObject Input;
            PackageResourceObject Output;
            Execution::Context& ParentContext;
            std::unique_ptr<Execution::Context> SubContext;

            // Reset the state that is modified by Get
            void Reset()
            {
                Output = Input.CopyForOutput();

                SubContext = ParentContext.CreateSubContext();
                SubContext->SetFlags(Execution::ContextFlag::DisableInteractivity);

                if (Input.AcceptAgreements().value_or(false))
                {
                    SubContext->Args.AddArg(Execution::Args::Type::AcceptSourceAgreements);
                    SubContext->Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);
                }
            }

            void PrepareSubContextInputs()
            {
                if (Input.Source())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::Source, Input.Source().value());
                }

                if (Input.Scope() && !Input.Scope().value().empty())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::InstallScope, ConvertScope(Input.Scope().value(), false));
                }
            }

            // Fills the Output object with the current state
            bool Get()
            {
                PrepareSubContextInputs();

                *SubContext <<
                    Workflow::ReportExecutionStage(Workflow::ExecutionStage::Discovery) <<
                    Workflow::OpenSource() <<
                    Workflow::OpenCompositeSource(Workflow::DetermineInstalledSource(*SubContext), false, CompositeSearchBehavior::AllPackages);

                if (SubContext->IsTerminated())
                {
                    ParentContext.Terminate(SubContext->GetTerminationHR());
                    return false;
                }

                // Do a manual search of the now opened source
                Source& source = SubContext->Get<Execution::Data::Source>();
                MatchType matchType = ToMatchType(Input.MatchOption()).value_or(MatchType::CaseInsensitive);

                SearchRequest request;
                request.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, matchType, Input.Identifier().value()));

                SearchResult result = source.Search(request);
                SubContext->Add<Execution::Data::SearchResult>(result);

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
                    SubContext->Add<Execution::Data::Package>(package);

                    auto installedPackage = package->GetInstalled();

                    // Fill Output and SubContext
                    Output.Exist(static_cast<bool>(installedPackage));
                    Output.Identifier(package->GetProperty(PackageProperty::Id));

                    if (installedPackage)
                    {
                        auto installedVersion = installedPackage->GetLatestVersion();
                        THROW_HR_IF(E_UNEXPECTED, !installedVersion);
                        auto metadata = installedVersion->GetMetadata();

                        Output.Version(installedVersion->GetProperty(PackageVersionProperty::Version));

                        auto scopeItr = metadata.find(PackageVersionMetadata::InstalledScope);
                        if (scopeItr != metadata.end())
                        {
                            Output.Scope(ConvertScope(scopeItr->second, true));
                        }

                        auto data = Repository::GetLatestApplicableVersion(package);
                        Output.UseLatest(!data.UpdateAvailable);
                    }
                }

                return true;
            }

            void Uninstall()
            {
                if (Input.Version())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::TargetVersion, Input.Version().value());
                }
                else
                {
                    SubContext->Args.AddArg(Execution::Args::Type::AllVersions);
                }

                *SubContext <<
                    Workflow::UninstallSinglePackage;

                if (SubContext->IsTerminated())
                {
                    ParentContext.Terminate(SubContext->GetTerminationHR());
                    return;
                }

                Output.Exist(false);
                Output.Version(std::nullopt);
                Output.Scope(std::nullopt);
                Output.UseLatest(std::nullopt);
            }

            void Install()
            {
                if (Input.Version())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::Version, Input.Version().value());
                }

                *SubContext <<
                    Workflow::SelectSinglePackageVersionForInstallOrUpgrade(Workflow::OperationType::Install) <<
                    Workflow::InstallSinglePackage;

                if (SubContext->IsTerminated())
                {
                    ParentContext.Terminate(SubContext->GetTerminationHR());
                    return;
                }

                Output.Exist(true);

                Output.Version(std::nullopt);
                if (SubContext->Contains(Execution::Data::PackageVersion))
                {
                    const auto& packageVersion = SubContext->Get<Execution::Data::PackageVersion>();
                    if (packageVersion)
                    {
                        Output.Version(packageVersion->GetProperty(Repository::PackageVersionProperty::Version));
                    }
                }

                Output.Scope(std::nullopt);
                if (SubContext->Contains(Execution::Data::Installer))
                {
                    const auto& installer = SubContext->Get<Execution::Data::Installer>();
                    if (installer)
                    {
                        Output.Scope(ConvertScope(Manifest::ScopeToString(installer->Scope), true));
                    }
                }

                Output.UseLatest(std::nullopt);
            }

            void Reinstall()
            {
                SubContext->Args.AddArg(Execution::Args::Type::UninstallPrevious);

                Install();
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
                        return TestVersion() && TestScope() && TestLatest();
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
                    if (!TestVersion())
                    {
                        result.append(std::string{ VersionProperty::Name() });
                    }

                    if (!TestScope())
                    {
                        result.append(std::string{ ScopeProperty::Name() });
                    }

                    if (!TestLatest())
                    {
                        result.append(std::string{ UseLatestProperty::Name() });
                    }
                }

                return result;
            }

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
                        return Manifest::ConvertToScopeEnum(ConvertScope(Input.Scope().value(), false)) ==
                            Manifest::ConvertToScopeEnum(ConvertScope(Output.Scope().value(), false));
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
                if (Input.UseLatest() && Input.UseLatest().value())
                {
                    if (Output.UseLatest())
                    {
                        return Output.UseLatest().value();
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
        };
    }

    DscPackageResource::DscPackageResource(std::string_view parent) :
        DscCommandBase(parent, "package", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
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

            if (!data.Get())
            {
                return;
            }

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscPackageResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            PackageFunctionData data{ context, json };

            if (!data.Get())
            {
                return;
            }

            // Capture the diff before updating the output
            auto diff = data.DiffJson();

            if (!data.Test())
            {
                if (data.Input.ShouldExist())
                {
                    if (data.Output.Exist().value())
                    {
                        if (!data.TestScope())
                        {
                            data.Reinstall();
                        }
                        if (!data.TestLatest())
                        {
                            // Install will swap to update flow
                            data.Install();
                        }
                        else // (!data.TestVersion())
                        {
                            Utility::Version inputVersion{ data.Input.Version().value() };
                            Utility::Version outputVersion{ data.Output.Version().value() };

                            if (outputVersion < inputVersion)
                            {
                                // Install will swap to update flow
                                data.Install();
                            }
                            else
                            {
                                data.Reinstall();
                            }
                        }
                    }
                    else
                    {
                        data.Install();
                    }
                }
                else
                {
                    data.Uninstall();
                }

                if (data.SubContext->IsTerminated())
                {
                    return;
                }
            }

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, diff);
        }
    }

    void DscPackageResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            PackageFunctionData data{ context, json };

            if (!data.Get())
            {
                return;
            }

            data.Output.InDesiredState(data.Test());

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, data.DiffJson());
        }
    }

    void DscPackageResource::ResourceFunctionExport(Execution::Context& context) const
    {
        auto json = GetJsonFromInput(context, false);
        PackageFunctionData data{ context, json, true };

        data.PrepareSubContextInputs();

        if (!data.Input.UseLatest().value_or(true))
        {
            data.SubContext->Args.AddArg(Execution::Args::Type::IncludeVersions);
        }

        data.SubContext->Args.AddArg(Execution::Args::Type::ConfigurationExportAll);

        *data.SubContext <<
            Workflow::SearchSourceForPackageExport;

        if (data.SubContext->IsTerminated())
        {
            context.Terminate(data.SubContext->GetTerminationHR());
            return;
        }

        const auto& packageCollection = data.SubContext->Get<Execution::Data::PackageCollection>();

        for (const auto& source : packageCollection.Sources)
        {
            for (const auto& package : source.Packages)
            {
                PackageResourceObject output;

                output.Identifier(package.Id);
                output.Source(source.Details.Name);

                if (!package.VersionAndChannel.GetVersion().IsEmpty())
                {
                    output.Version(package.VersionAndChannel.GetVersion().ToString());
                }

                if (package.Scope != Manifest::ScopeEnum::Unknown)
                {
                    output.Scope(ConvertScope(Manifest::ScopeToString(package.Scope), true));
                }

                WriteJsonOutputLine(context, output.ToJson());
            }
        }
    }

    void DscPackageResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, PackageResourceObject::Schema(ResourceType()));
    }
}
