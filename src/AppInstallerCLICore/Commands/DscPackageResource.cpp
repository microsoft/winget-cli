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
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(MatchOptionProperty, std::string, MatchOption, "matchOption", Resource::String::DscResourcePropertyDescriptionPackageMatchOption, ({ "equals", "equalsCaseInsensitive", "startsWithCaseInsensitive", "containsCaseInsensitive" }), "equalsCaseInsensitive");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_DEFAULT(UseLatestProperty, bool, UseLatest, "useLatest", Resource::String::DscResourcePropertyDescriptionPackageUseLatest, "false");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(InstallModeProperty, std::string, InstallMode, "installMode", Resource::String::DscResourcePropertyDescriptionPackageInstallMode, ({ "default", "silent", "interactive" }), "silent");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(AcceptAgreementsProperty, bool, AcceptAgreements, "acceptAgreements", Resource::String::DscResourcePropertyDescriptionPackageAcceptAgreements);

        // TODO: To support Scope on this resource:
        //  1. Change the installed source to pull in all package info for both scopes by default
        //  2. Change the installed source open in workflows to always open for everything, regardless of scope
        //  3. Improve correlation handling if needed for cross-scope package installations
        //  4. Update the test EXE installer to handle being installed for both scopes
        using PackageResourceObject = DscComposableObject<StandardExistProperty, StandardInDesiredStateProperty, IdProperty, SourceProperty, VersionProperty, MatchOptionProperty, UseLatestProperty, InstallModeProperty, AcceptAgreementsProperty>;

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
                        auto versionKeys = installedPackage->GetVersionKeys();
                        AICLI_LOG(CLI, Verbose, << "Package::Get found " << versionKeys.size() << " installed versions");

                        std::shared_ptr<Repository::IPackageVersion> installedVersion;

                        // Find the specific version provided if possible
                        if (Input.Version())
                        {
                            Utility::Version inputVersion{ Input.Version().value() };

                            for (const auto& key : versionKeys)
                            {
                                if (inputVersion == Utility::Version{ key.Version })
                                {
                                    installedVersion = installedPackage->GetVersion(key);
                                    break;
                                }
                            }
                        }

                        if (!installedVersion)
                        {
                            installedVersion = installedPackage->GetLatestVersion();
                        }

                        if (installedVersion)
                        {
                            Output.Version(installedVersion->GetProperty(PackageVersionProperty::Version));
                        }

                        auto data = Repository::GetLatestApplicableVersion(package);
                        Output.UseLatest(!data.UpdateAvailable);
                    }
                }

                AICLI_LOG(CLI, Verbose, << "Package::Get found:\n" << Json::writeString(Json::StreamWriterBuilder{}, Output.ToJson()));
                return true;
            }

            void Uninstall()
            {
                AICLI_LOG(CLI, Verbose, << "Package::Uninstall invoked");

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
                Output.UseLatest(std::nullopt);
            }

            void Install(bool allowDowngrade = false)
            {
                AICLI_LOG(CLI, Verbose, << "Package::Install invoked");

                if (Input.Version())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::Version, Input.Version().value());
                }

                *SubContext <<
                    Workflow::SelectSinglePackageVersionForInstallOrUpgrade(Workflow::OperationType::Install, allowDowngrade) <<
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

                Output.UseLatest(std::nullopt);
            }

            void Reinstall()
            {
                AICLI_LOG(CLI, Verbose, << "Package::Reinstall invoked");

                SubContext->Args.AddArg(Execution::Args::Type::UninstallPrevious);

                Install(true);
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
                        AICLI_LOG(CLI, Verbose, << "Package::Test needed to inspect these properties: Version(" << TestVersion() << "), Latest(" << TestLatest() << ")");
                        return TestVersion() && TestLatest();
                    }
                    else
                    {
                        AICLI_LOG(CLI, Verbose, << "Package::Test was false because the package was not installed");
                        return false;
                    }
                }
                else
                {
                    AICLI_LOG(CLI, Verbose, << "Package::Test desired the package to not exist, and it " << (Output.Exist().value() ? "did" : "did not"));
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
                        if (!data.TestLatest())
                        {
                            // Install will swap to update flow
                            AICLI_LOG(CLI, Info, << "Installing package to update to latest");
                            data.Install();
                        }
                        else // (!data.TestVersion())
                        {
                            Utility::Version inputVersion{ data.Input.Version().value() };
                            Utility::Version outputVersion{ data.Output.Version().value() };

                            if (outputVersion < inputVersion)
                            {
                                // Install will swap to update flow
                                AICLI_LOG(CLI, Info, << "Installing package to update to desired version");
                                data.Install();
                            }
                            else
                            {
                                AICLI_LOG(CLI, Info, << "Reinstalling package to downgrade to desired version");
                                data.Reinstall();
                            }
                        }
                    }
                    else
                    {
                        AICLI_LOG(CLI, Info, << "Installing package as it was not found");
                        data.Install();
                    }
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Uninstalling package as desired");
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

                // TODO: Exporting scope requires one or more of the following:
                //  1. Support for "OrUnknown" scope variants during Set (and workflows)
                //  2. Tracking scope intent as we do for some other installer properties
                //  3. Checking for the availability of the current scope in the package

                WriteJsonOutputLine(context, output.ToJson());
            }
        }
    }

    void DscPackageResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, PackageResourceObject::Schema(ResourceType()));
    }
}
