// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscSourceResource.h"
#include "DscComposableObject.h"
#include "Resources.h"
#include "Workflows/SourceFlow.h"
#include <winget/RepositorySource.h>

using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI
{
    namespace
    {
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_FLAGS(NameProperty, std::string, SourceName, "name", DscComposablePropertyFlag::Required | DscComposablePropertyFlag::CopyToOutput, Resource::String::DscResourcePropertyDescriptionSourceName);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(ArgumentProperty, std::string, Argument, "argument", Resource::String::DscResourcePropertyDescriptionSourceArgument);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(TypeProperty, std::string, Type, "type", Resource::String::DscResourcePropertyDescriptionSourceType);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY_ENUM(TrustLevelProperty, std::string, TrustLevel, "trustLevel", Resource::String::DscResourcePropertyDescriptionSourceTrustLevel, ({ "undefined", "none", "trusted" }), "undefined");
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(ExplicitProperty, bool, Explicit, "explicit", Resource::String::DscResourcePropertyDescriptionSourceExplicit);
        WINGET_DSC_DEFINE_COMPOSABLE_PROPERTY(AcceptAgreementsProperty, bool, AcceptAgreements, "acceptAgreements", Resource::String::DscResourcePropertyDescriptionAcceptAgreements);

        using SourceResourceObject = DscComposableObject<StandardExistProperty, StandardInDesiredStateProperty, NameProperty, ArgumentProperty, TypeProperty, TrustLevelProperty, ExplicitProperty, AcceptAgreementsProperty>;

        std::string TrustLevelStringFromFlags(SourceTrustLevel trustLevel)
        {
            return WI_IsFlagSet(trustLevel, SourceTrustLevel::Trusted) ? "trusted" : "none";
        }

        // The values as the resource uses them.
        enum class ResourceTrustLevel
        {
            Undefined,
            Invalid,
            None,
            Trusted
        };

        ResourceTrustLevel EffectiveTrustLevel(const std::optional<std::string>& input)
        {
            if (!input)
            {
                return ResourceTrustLevel::Undefined;
            }

            std::string inputValue = Utility::ToLower(input.value());
            if (inputValue == "undefined")
            {
                return ResourceTrustLevel::Undefined;
            }
            else if (inputValue == "none")
            {
                return ResourceTrustLevel::None;
            }
            else if (inputValue == "trusted")
            {
                return ResourceTrustLevel::Trusted;
            }
            else
            {
                return ResourceTrustLevel::Invalid;
            }
        }

        struct SourceFunctionData
        {
            SourceFunctionData(Execution::Context& context, const std::optional<Json::Value>& json, bool ignoreFieldRequirements = false) :
                Input(json, ignoreFieldRequirements),
                ParentContext(context)
            {
                Reset();
            }

            const SourceResourceObject Input;
            SourceResourceObject Output;
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
                }
            }

            // Fills the Output object with the current state
            void Get()
            {
                auto currentSources = Repository::Source::GetCurrentSources();
                const std::string& name = Input.SourceName().value();

                Output.Exist(false);

                for (auto const& source : currentSources)
                {
                    if (Utility::ICUCaseInsensitiveEquals(source.Name, name))
                    {
                        Output.Exist(true);
                        Output.Argument(source.Arg);
                        Output.Type(source.Type);
                        Output.TrustLevel(TrustLevelStringFromFlags(source.TrustLevel));
                        Output.Explicit(source.Explicit);

                        std::vector<Repository::SourceDetails> sources;
                        sources.emplace_back(source);
                        SubContext->Add<Execution::Data::SourceList>(std::move(sources));
                        break;
                    }
                }

                AICLI_LOG(CLI, Verbose, << "Source::Get found:\n" << Json::writeString(Json::StreamWriterBuilder{}, Output.ToJson()));
            }

            void Add()
            {
                AICLI_LOG(CLI, Verbose, << "Source::Add invoked");

                if (!SubContext->Args.Contains(Execution::Args::Type::SourceName))
                {
                    SubContext->Args.AddArg(Execution::Args::Type::SourceName, Input.SourceName().value());
                }

                THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, !Input.Argument().has_value());
                SubContext->Args.AddArg(Execution::Args::Type::SourceArg, Input.Argument().value());

                if (Input.Type())
                {
                    SubContext->Args.AddArg(Execution::Args::Type::SourceType, Input.Type().value());
                }

                ResourceTrustLevel effectiveTrustLevel = EffectiveTrustLevel(Input.TrustLevel());
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_INVALID_CL_ARGUMENTS, effectiveTrustLevel == ResourceTrustLevel::Invalid);
                if (effectiveTrustLevel == ResourceTrustLevel::Trusted)
                {
                    SubContext->Args.AddArg(Execution::Args::Type::SourceTrustLevel, TrustLevelStringFromFlags(SourceTrustLevel::Trusted));
                }

                if (Input.Explicit().value_or(false))
                {
                    SubContext->Args.AddArg(Execution::Args::Type::SourceExplicit);
                }

                *SubContext <<
                    Workflow::EnsureRunningAsAdmin <<
                    Workflow::CreateSourceForSourceAdd <<
                    Workflow::AddSource;
            }

            void Remove()
            {
                AICLI_LOG(CLI, Verbose, << "Source::Remove invoked");

                if (!SubContext->Args.Contains(Execution::Args::Type::SourceName))
                {
                    SubContext->Args.AddArg(Execution::Args::Type::SourceName, Input.SourceName().value());
                }

                *SubContext <<
                    Workflow::EnsureRunningAsAdmin <<
                    Workflow::RemoveSources;
            }

            void Replace()
            {
                AICLI_LOG(CLI, Verbose, << "Source::Replace invoked");
                Remove();
                Add();
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
                        AICLI_LOG(CLI, Verbose, << "Source::Test needed to inspect these properties: Argument(" << TestArgument() << "), Type(" << TestType() << "), TrustLevel(" << TestTrustLevel() << "), Explicit(" << TestExplicit() << ")");
                        return TestArgument() && TestType() && TestTrustLevel() && TestExplicit();
                    }
                    else
                    {
                        AICLI_LOG(CLI, Verbose, << "Source::Test was false because the source is not present");
                        return false;
                    }
                }
                else
                {
                    AICLI_LOG(CLI, Verbose, << "Source::Test desired the source to not exist, and it " << (Output.Exist().value() ? "did" : "did not"));
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
                    if (!TestArgument())
                    {
                        result.append(std::string{ ArgumentProperty::Name() });
                    }

                    if (!TestType())
                    {
                        result.append(std::string{ TypeProperty::Name() });
                    }

                    if (!TestTrustLevel())
                    {
                        result.append(std::string{ TrustLevelProperty::Name() });
                    }

                    if (!TestExplicit())
                    {
                        result.append(std::string{ ExplicitProperty::Name() });
                    }
                }

                return result;
            }

            bool TestArgument()
            {
                if (Input.Argument())
                {
                    if (Output.Argument())
                    {
                        return Input.Argument().value() == Output.Argument().value();
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

            bool TestType()
            {
                if (Input.Type())
                {
                    if (Output.Type())
                    {
                        return Utility::CaseInsensitiveEquals(Input.Type().value(), Output.Type().value());
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

            bool TestTrustLevel()
            {
                auto inputTrustLevel = EffectiveTrustLevel(Input.TrustLevel());

                if (inputTrustLevel != ResourceTrustLevel::Undefined)
                {
                    return inputTrustLevel == EffectiveTrustLevel(Output.TrustLevel());
                }
                else
                {
                    return true;
                }
            }

            bool TestExplicit()
            {
                if (Input.Explicit())
                {
                    if (Output.Explicit())
                    {
                        return Input.Explicit().value() == Output.Explicit().value();
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

    DscSourceResource::DscSourceResource(std::string_view parent) :
        DscCommandBase(parent, "source", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::HandlesExist | DscFunctionModifiers::ReturnsStateAndDiff)
    {
    }

    Resource::LocString DscSourceResource::ShortDescription() const
    {
        return Resource::String::DscSourceResourceShortDescription;
    }

    Resource::LocString DscSourceResource::LongDescription() const
    {
        return Resource::String::DscSourceResourceLongDescription;
    }

    std::string DscSourceResource::ResourceType() const
    {
        return "Source";
    }

    void DscSourceResource::ResourceFunctionGet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            SourceFunctionData data{ context, json };

            data.Get();

            WriteJsonOutputLine(context, data.Output.ToJson());
        }
    }

    void DscSourceResource::ResourceFunctionSet(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            SourceFunctionData data{ context, json };

            data.Get();

            // Capture the diff before updating the output
            auto diff = data.DiffJson();

            if (!data.Test())
            {
                if (data.Input.ShouldExist())
                {
                    if (data.Output.Exist().value())
                    {
                        AICLI_LOG(CLI, Info, << "Replacing source with new information");
                        data.Replace();
                    }
                    else
                    {
                        AICLI_LOG(CLI, Info, << "Adding source as it was not found");
                        data.Add();
                    }
                }
                else
                {
                    AICLI_LOG(CLI, Info, << "Removing source as desired");
                    data.Remove();
                }

                if (data.SubContext->IsTerminated())
                {
                    data.ParentContext.Terminate(data.SubContext->GetTerminationHR());
                    return;
                }

                data.Reset();
                data.Get();
            }

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, diff);
        }
    }

    void DscSourceResource::ResourceFunctionTest(Execution::Context& context) const
    {
        if (auto json = GetJsonFromInput(context))
        {
            SourceFunctionData data{ context, json };

            data.Get();
            data.Output.InDesiredState(data.Test());

            WriteJsonOutputLine(context, data.Output.ToJson());
            WriteJsonOutputLine(context, data.DiffJson());
        }
    }

    void DscSourceResource::ResourceFunctionExport(Execution::Context& context) const
    {
        auto currentSources = Repository::Source::GetCurrentSources();

        for (auto const& source : currentSources)
        {
            SourceResourceObject output;
            output.SourceName(source.Name);
            output.Argument(source.Arg);
            output.Type(source.Type);
            output.TrustLevel(TrustLevelStringFromFlags(source.TrustLevel));
            output.Explicit(source.Explicit);
            WriteJsonOutputLine(context, output.ToJson());
        }
    }

    void DscSourceResource::ResourceFunctionSchema(Execution::Context& context) const
    {
        WriteJsonOutputLine(context, SourceResourceObject::Schema(ResourceType()));
    }
}
