// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationFlow.h"
#include "PromptFlow.h"
#include <winrt/Microsoft.Management.Configuration.h>

using namespace AppInstaller::CLI::Execution;
using namespace winrt::Microsoft::Management::Configuration;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        constexpr std::wstring_view s_Directive_Description = L"description";
        constexpr std::wstring_view s_Directive_Module = L"module";

        Logging::Level ConvertLevel(DiagnosticLevel level)
        {
            switch (level)
            {
            case DiagnosticLevel::Verbose: return Logging::Level::Verbose;
            case DiagnosticLevel::Informational: return Logging::Level::Info;
            case DiagnosticLevel::Warning: return Logging::Level::Warning;
            case DiagnosticLevel::Error: return Logging::Level::Error;
            case DiagnosticLevel::Critical: return Logging::Level::Crit;
            }

            return Logging::Level::Info;
        }

        Resource::StringId ToResource(ConfigurationUnitIntent intent)
        {
            switch (intent)
            {
            case ConfigurationUnitIntent::Assert: return Resource::String::ConfigurationAssert;
            case ConfigurationUnitIntent::Inform: return Resource::String::ConfigurationInform;
            case ConfigurationUnitIntent::Apply: return Resource::String::ConfigurationApply;
            default: return Resource::StringId::Empty();
            }
        }

        std::optional<Utility::LocIndString> GetValueSetString(const ValueSet& valueSet, std::wstring_view value)
        {
            if (valueSet.HasKey(value))
            {
                auto object = valueSet.Lookup(value);
                IPropertyValue property = object.try_as<IPropertyValue>();
                if (property && property.Type() == PropertyType::String)
                {
                    return Utility::LocIndString{ Utility::ConvertToUTF8(property.GetString()) };
                }
            }

            return {};
        }

        void OutputValueSet(OutputStream& out, const ValueSet& valueSet, size_t indent)
        {
            Utility::LocIndString indentString{ std::string(indent, ' ') };

            for (const auto& value : valueSet)
            {
                out << indentString << Utility::ConvertToUTF8(value.Key()) << ':';

                auto object = value.Value();

                IPropertyValue property = object.try_as<IPropertyValue>();
                if (property)
                {
                    switch (property.Type())
                    {
                    case PropertyType::String:
                        out << ' ' << Utility::ConvertToUTF8(property.GetString()) << '\n';
                        break;
                    default:
                        // TODO: Sort out how we actually want to handle this given that we don't expect anything but strings
                        out << " [PropertyType="_liv << property.Type() << "]\n"_liv;
                        break;
                    }
                }
                else
                {
                    // If not an IPropertyValue, it must be a ValueSet
                    ValueSet subset = object.as<ValueSet>();
                    out << '\n';
                    OutputValueSet(out, subset, indent + 2);
                }
            }
        }

        // Converts a string from the configuration API surface for output.
        // All strings coming from the API are external data and not localizable by us.
        Utility::LocIndString ConvertForOutput(const winrt::hstring& input)
        {
            return Utility::LocIndString{ Utility::ConvertToUTF8(input) };
        }

        void OutputConfigurationUnitHeader(OutputStream& out, const ConfigurationUnit& unit, const winrt::hstring& name)
        {
            out << ConfigurationIntentEmphasis << ToResource(unit.Intent()) << " :: "_liv << ConfigurationUnitEmphasis << ConvertForOutput(name);

            winrt::hstring identifier = unit.Identifier();
            if (!identifier.empty())
            {
                out << " ["_liv << ConvertForOutput(identifier) << ']';
            }

            out << '\n';
        }

        void OutputConfigurationUnitInformation(OutputStream& out, const ConfigurationUnit& unit)
        {
            IConfigurationUnitProcessorDetails details = unit.Details();
            ValueSet directives = unit.Directives();

            if (details)
            {
                // -- Sample output when IConfigurationUnitProcessorDetails present --
                // Intent :: UnitName <from details> [Identifier]
                //   UnitDocumentationUri <if present>
                //   Description <from details first, directives second>
                //   "Module": ModuleName "by" Author / Publisher (IsLocal / ModuleSource)
                //     "Signed by": SigningCertificateChain (leaf subject CN)
                //     PublishedModuleUri / ModuleDocumentationUri <if present>
                //     ModuleDescription
                OutputConfigurationUnitHeader(out, unit, details.UnitName());

                auto unitDocumentationUri = details.UnitDocumentationUri();
                if (unitDocumentationUri)
                {
                    out << "  "_liv << ConvertForOutput(unitDocumentationUri.DisplayUri()) << '\n';
                }

                winrt::hstring unitDescriptionFromDetails = details.UnitDescription();
                if (!unitDescriptionFromDetails.empty())
                {
                    out << "  "_liv << ConvertForOutput(unitDescriptionFromDetails) << '\n';
                }
                else
                {
                    auto unitDescriptionFromDirectives = GetValueSetString(directives, s_Directive_Description);
                    if (unitDescriptionFromDirectives && !unitDescriptionFromDirectives.value().empty())
                    {
                        out << "  "_liv << unitDescriptionFromDirectives.value() << '\n';
                    }
                }

                auto author = ConvertForOutput(details.Author());
                if (author.empty())
                {
                    author = ConvertForOutput(details.Publisher());
                }
                if (details.IsLocal())
                {
                    out << "  "_liv << Resource::String::ConfigurationModuleWithDetails(ConvertForOutput(details.ModuleName()), author, Resource::String::ConfigurationLocal) << '\n';
                }
                else
                {
                    out << "  "_liv << Resource::String::ConfigurationModuleWithDetails(ConvertForOutput(details.ModuleName()), author, ConvertForOutput(details.ModuleSource())) << '\n';
                }

                // TODO: Signing information after it gets changed

                auto moduleUri = details.PublishedModuleUri();
                if (!moduleUri)
                {
                    moduleUri = details.ModuleDocumentationUri();
                }
                if (moduleUri)
                {
                    out << "    "_liv << ConvertForOutput(moduleUri.DisplayUri()) << '\n';
                }

                winrt::hstring moduleDescription = details.ModuleDescription();
                if (!moduleDescription.empty())
                {
                    out << "    "_liv << ConvertForOutput(moduleDescription) << '\n';
                }
            }
            else
            {
                // -- Sample output when no IConfigurationUnitProcessorDetails present --
                // Intent :: UnitName <from unit> [identifier]
                //   Description (from directives)
                //   "Module": module <directive>
                OutputConfigurationUnitHeader(out, unit, unit.UnitName());

                auto description = GetValueSetString(directives, s_Directive_Description);
                if (description && !description.value().empty())
                {
                    out << "  "_liv << description.value() << '\n';
                }

                auto module = GetValueSetString(directives, s_Directive_Module);
                if (module && !module.value().empty())
                {
                    out << "  "_liv << Resource::String::ConfigurationModuleNameOnly(module.value()) << '\n';
                }
            }

            // -- Sample output footer --
            //   Dependencies: dep1, dep2, ...
            //   Settings:
            //     <... settings splat>
            auto dependencies = unit.Dependencies();
            if (dependencies.Size() > 0)
            {
                std::ostringstream allDependencies;
                for (const winrt::hstring& dependency : dependencies)
                {
                    allDependencies << ' ' << Utility::ConvertToUTF8(dependency);
                }
                out << "  "_liv << Resource::String::ConfigurationDependencies(Utility::LocIndString{ std::move(allDependencies).str() }) << '\n';
            }

            ValueSet settings = unit.Settings();
            if (settings.Size() > 0)
            {
                out << "  "_liv << Resource::String::ConfigurationSettings << '\n';
                OutputValueSet(out, settings, 4);
            }
        }

        void LogFailedGetConfigurationUnitDetails(const ConfigurationUnit& unit, const ConfigurationUnitResultInformation& resultInformation)
        {
            if (FAILED(resultInformation.ResultCode()))
            {
                AICLI_LOG(Config, Error, << "Failed to get unit details for " << Utility::ConvertToUTF8(unit.UnitName()) << " : 0x" <<
                    Logging::SetHRFormat << resultInformation.ResultCode() << '\n' << Utility::ConvertToUTF8(resultInformation.Description()));
            }
        }

        // Helper to handle progress callbacks from ApplyConfigurationSetAsync
        struct ApplyConfigurationSetProgressOutput
        {
            ApplyConfigurationSetProgressOutput(Context& context) : m_context(context) {}

            void Progress(const IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData>& operation, const ConfigurationSetChangeData& data)
            {
                if (m_isFirstProgress)
                {
                    HandleUnreportedProgress(operation.GetResults());
                }

                switch (data.Change())
                {
                case ConfigurationSetChangeEventType::SetStateChanged:
                {
                    switch (data.SetState())
                    {
                    case ConfigurationSetState::Pending:
                        m_context.Reporter.Info() << Resource::String::ConfigurationWaitingOnAnother << std::endl;
                        m_context.Reporter.BeginProgress();
                        break;
                    case ConfigurationSetState::InProgress:
                        m_context.Reporter.EndProgress(true);
                        break;
                    case ConfigurationSetState::Completed:
                        m_context.Reporter.EndProgress(true);
                        break;
                    }
                }
                    break;
                case ConfigurationSetChangeEventType::UnitStateChanged:
                    HandleUnitProgress(data.Unit(), data.UnitState(), data.ResultInformation());
                    break;
                }
            }

            // If no progress has been reported, this function will report the given results
            void HandleUnreportedProgress(const ApplyConfigurationSetResult& result)
            {
                if (m_isFirstProgress)
                {
                    m_isFirstProgress = false;

                    for (const ApplyConfigurationUnitResult& unitResult : result.UnitResults())
                    {
                        HandleUnitProgress(unitResult.Unit(), unitResult.State(), unitResult.ResultInformation());
                    }
                }
            }

        private:
            void HandleUnitProgress(const ConfigurationUnit& unit, ConfigurationUnitState state, const ConfigurationUnitResultInformation& resultInformation)
            {
                switch (state)
                {
                case ConfigurationUnitState::Pending:
                    // The unreported progress handler may send pending units, just ignore them
                    break;
                case ConfigurationUnitState::InProgress:
                    OutputUnitInProgressIfNeeded(unit);
                    m_context.Reporter.BeginProgress();
                    break;
                case ConfigurationUnitState::Completed:
                    OutputUnitInProgressIfNeeded(unit);
                    m_context.Reporter.EndProgress(true);
                    if (SUCCEEDED(resultInformation.ResultCode()))
                    {
                        m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationSuccessfullyApplied << std::endl;
                    }
                    else
                    {
                        AICLI_LOG(Config, Error, << "Configuration unit " << Utility::ConvertToUTF8(unit.UnitName()) << "[" << Utility::ConvertToUTF8(unit.Identifier()) << "] failed with code 0x"
                            << Logging::SetHRFormat << resultInformation.ResultCode() << " and error message:\n" << Utility::ConvertToUTF8(resultInformation.Description()));
                        m_context.Reporter.Error() << "  "_liv << Resource::String::ConfigurationUnitFailed << " 0x"_liv << Logging::SetHRFormat << resultInformation.ResultCode() << std::endl;
                    }
                    OutputUnitCompletionProgress();
                    break;
                case ConfigurationUnitState::Skipped:
                    OutputUnitInProgressIfNeeded(unit);
                    AICLI_LOG(Config, Error, << "Configuration unit " << Utility::ConvertToUTF8(unit.UnitName()) << "[" << Utility::ConvertToUTF8(unit.Identifier()) << "] was skipped with code 0x"
                        << Logging::SetHRFormat << resultInformation.ResultCode());
                    // TODO: Unique message per skip reason?
                    m_context.Reporter.Warn() << "  "_liv << Resource::String::ConfigurationUnitSkipped << " 0x"_liv << Logging::SetHRFormat << resultInformation.ResultCode() << std::endl;
                    OutputUnitCompletionProgress();
                    break;
                }
            }

            void OutputUnitInProgressIfNeeded(const ConfigurationUnit& unit)
            {
                winrt::guid unitInstance = unit.InstanceIdentifier();
                if (m_unitsSeen.count(unitInstance) == 0)
                {
                    m_unitsSeen.insert(unitInstance);

                    OutputStream out = m_context.Reporter.Info();
                    OutputConfigurationUnitHeader(out, unit, unit.Details() ? unit.Details().UnitName() : unit.UnitName());
                }
            }

            // Sends VT progress to the console
            void OutputUnitCompletionProgress()
            {
                // TODO: Change progress reporting to enable separation of spinner and VT progress reporting
                //  Preferrably we want to be able to have:
                //      1. Spinner with indefinite progress VT before set application begins
                //      2. 1/N VT progress reporting for configuration units while also showing a spinner for the unit itself
            }

            Context& m_context;
            std::set<winrt::guid> m_unitsSeen;
            bool m_isFirstProgress = true;
        };
    }

    void CreateConfigurationProcessor(Context& context)
    {
        // TODO: Create the real factory
        IConfigurationSetProcessorFactory factory = nullptr;

        ConfigurationProcessor processor{ factory };

        // Route the configuration diagnostics into the context's diagnostics logging
        processor.Diagnostics([&context](const winrt::Windows::Foundation::IInspectable&, const DiagnosticInformation& diagnostics)
            {
                context.GetThreadGlobals().GetDiagnosticLogger().Write(Logging::Channel::Config, ConvertLevel(diagnostics.Level()), Utility::ConvertToUTF8(diagnostics.Message()));
            });

        context.Add<Data::ConfigurationContext>(ConfigurationContext{});
        context.Get<Data::ConfigurationContext>().Processor(std::move(processor));
    }

    void OpenConfigurationSet(Context& context)
    {
        Streams::IInputStream inputStream = nullptr;
        inputStream = Streams::FileRandomAccessStream::OpenAsync(Utility::ConvertToUTF16(context.Args.GetArg(Args::Type::ConfigurationFile)), FileAccessMode::Read).get();

        OpenConfigurationSetResult openResult = context.Get<Data::ConfigurationContext>().Processor().OpenConfigurationSet(inputStream);
        if (FAILED_LOG(static_cast<HRESULT>(openResult.ResultCode().value)))
        {
            switch (openResult.ResultCode())
            {
            case WINGET_CONFIG_ERROR_INVALID_FIELD:
                context.Reporter.Error() << Resource::String::ConfigurationFieldInvalid(Utility::LocIndString{ Utility::ConvertToUTF8(openResult.Field()) }) << std::endl;
                break;
            case WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION:
                context.Reporter.Error() << Resource::String::ConfigurationFileVersionUnknown(Utility::LocIndString{ Utility::ConvertToUTF8(openResult.Field()) }) << std::endl;
                break;
            case WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE:
            case WINGET_CONFIG_ERROR_INVALID_YAML:
            default:
                context.Reporter.Error() << Resource::String::ConfigurationFileInvalid << std::endl;
                break;
            }

            AICLI_TERMINATE_CONTEXT(openResult.ResultCode());
        }

        context.Get<Data::ConfigurationContext>().Set(openResult.Set());
    }

    void ShowConfigurationSet(Context& context)
    {
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();
        auto getDetailsOperation = configContext.Processor().GetSetDetailsAsync(configContext.Set(), ConfigurationUnitDetailLevel::Catalog);

        OutputStream out = context.Reporter.Info();
        uint32_t unitsShown = 0;

        getDetailsOperation.Progress([&](const IAsyncOperationWithProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult>& operation, const GetConfigurationUnitDetailsResult&)
            {
                auto unitResults = operation.GetResults().UnitResults();
                for (unitsShown; unitsShown < unitResults.Size(); ++unitsShown)
                {
                    GetConfigurationUnitDetailsResult unitResult = unitResults.GetAt(unitsShown);
                    LogFailedGetConfigurationUnitDetails(unitResult.Unit(), unitResult.ResultInformation());
                    OutputConfigurationUnitInformation(out, unitResult.Unit());
                }
            });

        try
        {
            GetConfigurationSetDetailsResult result = getDetailsOperation.get();

            // Handle any missing progress callbacks
            auto unitResults = result.UnitResults();
            for (unitsShown; unitsShown < unitResults.Size(); ++unitsShown)
            {
                GetConfigurationUnitDetailsResult unitResult = unitResults.GetAt(unitsShown);
                LogFailedGetConfigurationUnitDetails(unitResult.Unit(), unitResult.ResultInformation());
                OutputConfigurationUnitInformation(out, unitResult.Unit());
            }
        }
        CATCH_LOG();

        // In the event of an exception from GetSetDetailsAsync, show the data we do have
        if (!unitsShown && configContext.Set().ConfigurationUnits().Size())
        {
            // Failing to get details might not be fatal, warn about it but proceed
            context.Reporter.Warn() << Resource::String::ConfigurationFailedToGetDetails << std::endl;

            for (const ConfigurationUnit& unit : configContext.Set().ConfigurationUnits())
            {
                OutputConfigurationUnitInformation(out, unit);
            }
        }
    }

    void ShowConfigurationSetConflicts(Execution::Context& context)
    {
        UNREFERENCED_PARAMETER(context);
    }

    void ConfirmConfigurationProcessing(Execution::Context& context)
    {
        context.Reporter.Warn() << Resource::String::ConfigurationWarning << std::endl;

        if (!context.Args.Contains(Args::Type::ConfigurationAcceptWarning))
        {
            context << RequireInteractivity(WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED);
            if (context.IsTerminated())
            {
                return;
            }

            if (!context.Reporter.PromptForBoolResponse(Resource::String::ConfigurationWarningPrompt, Reporter::Level::Warning))
            {
                AICLI_TERMINATE_CONTEXT(WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED);
            }
        }
    }

    void ApplyConfigurationSet(Execution::Context& context)
    {
        ApplyConfigurationSetProgressOutput progress{ context };
        ApplyConfigurationSetResult result = nullptr;

        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();

        {
            // Just in case, forcibly stop our manual progress
            auto hideProgress = wil::scope_exit([&]()
                {
                    context.Reporter.EndProgress(true);
                });

            auto applyOperation = configContext.Processor().ApplySetAsync(configContext.Set(), ApplyConfigurationSetFlags::None);

            applyOperation.Progress([&](const IAsyncOperationWithProgress<ApplyConfigurationSetResult, ConfigurationSetChangeData>& operation, const ConfigurationSetChangeData& data)
                {
                    progress.Progress(operation, data);
                });

            result = applyOperation.get();
            progress.HandleUnreportedProgress(result);
        }

        if (FAILED(result.ResultCode()))
        {
            context.Reporter.Error() << Resource::String::ConfigurationFailedToApply << std::endl;

            // TODO: Summarize failed configuration units, especially if we put more output for each one during execution

            AICLI_TERMINATE_CONTEXT(result.ResultCode());
        }
        else
        {
            context.Reporter.Info() << Resource::String::ConfigurationSuccessfullyApplied << std::endl;
        }
    }
}
