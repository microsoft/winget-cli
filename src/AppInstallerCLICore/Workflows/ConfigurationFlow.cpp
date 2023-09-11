// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationFlow.h"
#include "PromptFlow.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include <AppInstallerErrors.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <winget/SelfManagement.h>
#include "ConfigurationCommon.h"

using namespace AppInstaller::CLI::Execution;
using namespace winrt::Microsoft::Management::Configuration;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;
using namespace AppInstaller::Utility::literals;
using namespace AppInstaller::SelfManagement;

namespace AppInstaller::CLI::Workflow
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    IConfigurationSetProcessorFactory s_override_IConfigurationSetProcessorFactory;

    void SetTestConfigurationSetProcessorFactory(IConfigurationSetProcessorFactory factory)
    {
        s_override_IConfigurationSetProcessorFactory = std::move(factory);
    }
#endif

    namespace
    {
        constexpr std::wstring_view s_Directive_Description = L"description";
        constexpr std::wstring_view s_Directive_Module = L"module";
        constexpr std::wstring_view s_Directive_AllowPrerelease = L"allowPrerelease";

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

        DiagnosticLevel ConvertLevel(Logging::Level level)
        {
            switch (level)
            {
            case Logging::Level::Verbose: return DiagnosticLevel::Verbose;
            case Logging::Level::Info: return DiagnosticLevel::Informational;
            case Logging::Level::Warning: return DiagnosticLevel::Warning;
            case Logging::Level::Error: return DiagnosticLevel::Error;
            case Logging::Level::Crit: return DiagnosticLevel::Critical;
            }

            return DiagnosticLevel::Informational;
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

        IConfigurationSetProcessorFactory CreateConfigurationSetProcessorFactory(Execution::Context& context)
        {
#ifndef AICLI_DISABLE_TEST_HOOKS
            // Test could override the entire workflow task, but that may require keeping more in sync than simply setting the factory.
            if (s_override_IConfigurationSetProcessorFactory)
            {
                return s_override_IConfigurationSetProcessorFactory;
            }
#endif

            auto factory = ConfigurationRemoting::CreateOutOfProcessFactory();
            Configuration::SetModulePath(context, factory);
            return factory;
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

        std::optional<bool> GetValueSetBool(const ValueSet& valueSet, std::wstring_view value)
        {
            if (valueSet.HasKey(value))
            {
                auto object = valueSet.Lookup(value);
                IPropertyValue property = object.try_as<IPropertyValue>();
                if (property && property.Type() == PropertyType::Boolean)
                {
                    return property.GetBoolean();
                }
            }

            return {};
        }

        void OutputPropertyValue(OutputStream& out, const IPropertyValue property)
        {
            switch (property.Type())
            {
            case PropertyType::String:
                out << ' ' << Utility::ConvertToUTF8(property.GetString()) << '\n';
                break;
            case PropertyType::Boolean:
                out << ' ' << (property.GetBoolean() ? Utility::LocIndView("true") : Utility::LocIndView("false")) << '\n';
                break;
            case PropertyType::Int64:
                out << ' ' << property.GetInt64() << '\n';
                break;
            default:
                out << " [Debug:PropertyType="_liv << property.Type() << "]\n"_liv;
                break;
            }
        }

        void OutputValueSet(OutputStream& out, const ValueSet& valueSet, size_t indent);

        void OutputValueSetAsArray(OutputStream& out, const ValueSet& valueSetArray, size_t indent)
        {
            Utility::LocIndString indentString{ std::string(indent, ' ') };

            std::vector<std::pair<int, winrt::Windows::Foundation::IInspectable>> arrayValues;
            for (const auto& arrayValue : valueSetArray)
            {
                if (arrayValue.Key() != L"treatAsArray")
                {
                    arrayValues.emplace_back(std::make_pair(std::stoi(arrayValue.Key().c_str()), arrayValue.Value()));
                }
            }

            std::sort(
                arrayValues.begin(),
                arrayValues.end(),
                [](const std::pair<int, winrt::Windows::Foundation::IInspectable>& a, const std::pair<int, winrt::Windows::Foundation::IInspectable>& b)
                {
                    return a.first < b.first;
                });

            for (const auto& arrayValue : arrayValues)
            {
                auto arrayObject = arrayValue.second;
                IPropertyValue arrayProperty = arrayObject.try_as<IPropertyValue>();

                out << indentString << "-";
                if (arrayProperty)
                {
                    OutputPropertyValue(out, arrayProperty);
                }
                else
                {
                    ValueSet arraySubset = arrayObject.as<ValueSet>();
                    auto size = arraySubset.Size();
                    if (size > 0)
                    {
                        // First one is special.
                        auto first = arraySubset.First().Current();
                        out << ' ' << Utility::ConvertToUTF8(first.Key()) << ':';
                        
                        auto object = first.Value();
                        IPropertyValue property = object.try_as<IPropertyValue>();
                        if (property)
                        {
                            OutputPropertyValue(out, property);
                        }
                        else
                        {
                            // If not an IPropertyValue, it must be a ValueSet
                            ValueSet subset = object.as<ValueSet>();
                            out << '\n';
                            OutputValueSet(out, subset, indent + 4);
                        }

                        if (size > 1)
                        {
                            arraySubset.Remove(first.Key());
                            OutputValueSet(out, arraySubset, indent + 2);
                            arraySubset.Insert(first.Key(), first.Value());
                        }
                    }
                }
            }
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
                    OutputPropertyValue(out, property);
                }
                else
                {
                    // If not an IPropertyValue, it must be a ValueSet
                    ValueSet subset = object.as<ValueSet>();
                    out << '\n';
                    if (subset.HasKey(L"treatAsArray"))
                    {
                        OutputValueSetAsArray(out, subset, indent + 2);
                    }
                    else
                    {
                        OutputValueSet(out, subset, indent + 2);
                    }
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
            ValueSet metadata = unit.Metadata();

            if (details)
            {
                // -- Sample output when IConfigurationUnitProcessorDetails present --
                // Intent :: UnitType <from details> [Identifier]
                //   UnitDocumentationUri <if present>
                //   Description <from details first, directives second>
                //   "Module": ModuleName "by" Author / Publisher (IsLocal / ModuleSource)
                //     "Signed by": SigningCertificateChain (leaf subject CN)
                //     PublishedModuleUri / ModuleDocumentationUri <if present>
                //     ModuleDescription
                OutputConfigurationUnitHeader(out, unit, details.UnitType());

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
                    auto unitDescriptionFromDirectives = GetValueSetString(metadata, s_Directive_Description);
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

                // TODO: Currently the signature information is only for the top files. Maybe each item should be tagged?
                // TODO: Output signing information with additional details (like whether the certificate is trusted). Doing this with the validate command
                //       seems like a good time, as that will also need to do the check in order to inform the user on the validation.
                //       Just saying "Signed By: Foo" is going to lead to a false sense of trust if the signature is valid but not actually trusted.

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
                // Intent :: Type <from unit> [identifier]
                //   Description (from directives)
                //   "Module": module <directive>
                OutputConfigurationUnitHeader(out, unit, unit.Type());

                auto description = GetValueSetString(metadata, s_Directive_Description);
                if (description && !description.value().empty())
                {
                    out << "  "_liv << description.value() << '\n';
                }

                auto module = GetValueSetString(metadata, s_Directive_Module);
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

        void LogFailedGetConfigurationUnitDetails(const ConfigurationUnit& unit, const IConfigurationUnitResultInformation& resultInformation)
        {
            if (FAILED(resultInformation.ResultCode()))
            {
                AICLI_LOG(Config, Error, << "Failed to get unit details for " << Utility::ConvertToUTF8(unit.Type()) << " : 0x" <<
                    Logging::SetHRFormat << resultInformation.ResultCode() << '\n' << Utility::ConvertToUTF8(resultInformation.Description()) << '\n' <<
                    Utility::ConvertToUTF8(resultInformation.Details()));
            }
        }

        struct UnitFailedMessageData
        {
            Utility::LocIndString Message;
            bool ShowDescription = true;
        };

        // TODO: We may need a detailed result code to enable the internal error to be exposed.
        //       Additionally, some of the processor exceptions that generate these errors should be enlightened to produce better, localized descriptions.
        UnitFailedMessageData GetUnitFailedData(const ConfigurationUnit& unit, const IConfigurationUnitResultInformation& resultInformation)
        {
            int32_t resultCode = resultInformation.ResultCode();

            switch (resultCode)
            {
            case WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER: return { Resource::String::ConfigurationUnitHasDuplicateIdentifier(Utility::LocIndString{ Utility::ConvertToUTF8(unit.Identifier()) }), false };
            case WINGET_CONFIG_ERROR_MISSING_DEPENDENCY: return { Resource::String::ConfigurationUnitHasMissingDependency(Utility::LocIndString{ Utility::ConvertToUTF8(resultInformation.Details()) }), false };
            case WINGET_CONFIG_ERROR_ASSERTION_FAILED: return { Resource::String::ConfigurationUnitAssertHadNegativeResult(), false };
            case WINGET_CONFIG_ERROR_UNIT_NOT_INSTALLED: return { Resource::String::ConfigurationUnitNotFoundInModule(), false };
            case WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY: return { Resource::String::ConfigurationUnitNotFound(), false };
            case WINGET_CONFIG_ERROR_UNIT_MULTIPLE_MATCHES: return { Resource::String::ConfigurationUnitMultipleMatches(), false };
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_GET: return { Resource::String::ConfigurationUnitFailedDuringGet(), true };
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_TEST: return { Resource::String::ConfigurationUnitFailedDuringTest(), true };
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_SET: return { Resource::String::ConfigurationUnitFailedDuringSet(), true };
            case WINGET_CONFIG_ERROR_UNIT_MODULE_CONFLICT: return { Resource::String::ConfigurationUnitModuleConflict(), false };
            case WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE: return { Resource::String::ConfigurationUnitModuleImportFailed(), true };
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_INVALID_RESULT: return { Resource::String::ConfigurationUnitReturnedInvalidResult(), false };
            }

            switch (resultInformation.ResultSource())
            {
            case ConfigurationUnitResultSource::ConfigurationSet: return { Resource::String::ConfigurationUnitFailedConfigSet(resultCode), true };
            case ConfigurationUnitResultSource::Internal: return { Resource::String::ConfigurationUnitFailedInternal(resultCode), true };
            case ConfigurationUnitResultSource::Precondition: return { Resource::String::ConfigurationUnitFailedPrecondition(resultCode), true };
            case ConfigurationUnitResultSource::SystemState: return { Resource::String::ConfigurationUnitFailedSystemState(resultCode), true };
            case ConfigurationUnitResultSource::UnitProcessing: return { Resource::String::ConfigurationUnitFailedUnitProcessing(resultCode), true };
            }

            // All other errors use a generic message
            return { Resource::String::ConfigurationUnitFailed(resultCode), true };
        }

        Utility::LocIndString GetUnitSkippedMessage(const IConfigurationUnitResultInformation& resultInformation)
        {
            int32_t resultCode = resultInformation.ResultCode();

            switch (resultInformation.ResultCode())
            {
            case WINGET_CONFIG_ERROR_MANUALLY_SKIPPED: return Resource::String::ConfigurationUnitManuallySkipped();
            case WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED: return Resource::String::ConfigurationUnitNotRunDueToDependency();
            case WINGET_CONFIG_ERROR_ASSERTION_FAILED: return Resource::String::ConfigurationUnitNotRunDueToFailedAssert();
            }

            // If new cases arise and are not handled here, at least have a generic backstop message.
            return Resource::String::ConfigurationUnitSkipped(resultCode);
        }

        void OutputUnitRunFailure(Context& context, const ConfigurationUnit& unit, const IConfigurationUnitResultInformation& resultInformation)
        {
            std::string description = Utility::Trim(Utility::ConvertToUTF8(resultInformation.Description()));

            AICLI_LOG_LARGE_STRING(Config, Error, << "Configuration unit " << Utility::ConvertToUTF8(unit.Type()) << "[" << Utility::ConvertToUTF8(unit.Identifier()) << "] failed with code 0x"
                << Logging::SetHRFormat << resultInformation.ResultCode() << " and error message:\n" << description, Utility::ConvertToUTF8(resultInformation.Details()));

            UnitFailedMessageData messageData = GetUnitFailedData(unit, resultInformation);
            auto error = context.Reporter.Error();
            error << "  "_liv << messageData.Message << std::endl;

            if (messageData.ShowDescription && !description.empty())
            {
                constexpr size_t maximumDescriptionLines = 3;
                size_t consoleWidth = GetConsoleWidth();
                std::vector<std::string> lines = Utility::SplitIntoLines(description, maximumDescriptionLines + 1);
                bool wasLimited = Utility::LimitOutputLines(lines, consoleWidth, maximumDescriptionLines);

                for (const auto& line : lines)
                {
                    error << line << std::endl;
                }

                if (wasLimited || !resultInformation.Details().empty())
                {
                    error << Resource::String::ConfigurationDescriptionWasTruncated << std::endl;
                }
            }
        }

        // Coordinates an active progress scope and cancellation of the operation.
        template<typename OperationT>
        struct ProgressCancellationUnification
        {
            ProgressCancellationUnification(std::unique_ptr<Reporter::AsyncProgressScope>&& progressScope, const OperationT& operation) :
                m_progressScope(std::move(progressScope)), m_operation(operation)
            {
                SetCancellationFunction();
            }

            void Reset()
            {
                m_cancelScope.reset();
                m_progressScope.reset();
            }

            Reporter::AsyncProgressScope& Progress() const { return *m_progressScope; }

            void Progress(std::unique_ptr<Reporter::AsyncProgressScope>&& progressScope)
            {
                m_cancelScope.reset();
                m_progressScope = std::move(progressScope);
                SetCancellationFunction();
            }

            OperationT& Operation() const { return m_operation; }

        private:
            void SetCancellationFunction()
            {
                if (m_progressScope)
                {
                    m_cancelScope = m_progressScope->Callback().SetCancellationFunction([this]() { m_operation.Cancel(); });
                }
            }

            std::unique_ptr<Reporter::AsyncProgressScope> m_progressScope;
            OperationT m_operation;
            IProgressCallback::CancelFunctionRemoval m_cancelScope;
        };

        template<typename Operation>
        ProgressCancellationUnification<Operation> CreateProgressCancellationUnification(
            std::unique_ptr<Reporter::AsyncProgressScope>&& progressScope,
            const Operation& operation)
        {
            return { std::move(progressScope), operation };
        }

        // The base type for progress reporting
        template<typename ResultType, typename ProgressType>
        struct ConfigurationSetProgressOutputBase
        {
            using Operation = IAsyncOperationWithProgress<ResultType, ProgressType>;

            ConfigurationSetProgressOutputBase(Context& context, const Operation& operation) :
                m_context(context), m_unification({}, operation)
            {
                operation.Progress([&](const Operation& operation, const ProgressType& data)
                    {
                        Progress(operation, data);
                    });
            }

            virtual void Progress(const Operation& operation, const ProgressType& data) = 0;

        protected:
            void MarkCompleted(const ConfigurationUnit& unit)
            {
                winrt::guid unitInstance = unit.InstanceIdentifier();
                m_unitsCompleted.insert(unitInstance);
            }

            bool UnitHasPreviouslyCompleted(const ConfigurationUnit& unit)
            {
                winrt::guid unitInstance = unit.InstanceIdentifier();
                return m_unitsCompleted.count(unitInstance) != 0;
            }

            // Sends VT progress to the console
            void OutputUnitCompletionProgress()
            {
                // TODO: Change progress reporting to enable separation of spinner and VT progress reporting
                //  Preferably we want to be able to have:
                //      1. Spinner with indefinite progress VT before set application begins
                //      2. 1/N VT progress reporting for configuration units while also showing a spinner for the unit itself
            }

            void BeginProgress()
            {
                m_unification.Progress(m_context.Reporter.BeginAsyncProgress(true));
            }

            void EndProgress()
            {
                m_unification.Reset();
            }

            Context& m_context;

        private:
            ProgressCancellationUnification<Operation> m_unification;
            std::set<winrt::guid> m_unitsCompleted;
        };

        // Helper to handle progress callbacks from ApplyConfigurationSetAsync
        struct ApplyConfigurationSetProgressOutput final : public ConfigurationSetProgressOutputBase<ApplyConfigurationSetResult, ConfigurationSetChangeData>
        {
            using Operation = ConfigurationSetProgressOutputBase<ApplyConfigurationSetResult, ConfigurationSetChangeData>::Operation;

            ApplyConfigurationSetProgressOutput(Context& context, const Operation& operation) :
                ConfigurationSetProgressOutputBase(context, operation)
            {
            }

            void Progress(const Operation& operation, const ConfigurationSetChangeData& data) override
            {
                auto threadContext = m_context.SetForCurrentThread();

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
                        BeginProgress();
                        break;
                    case ConfigurationSetState::InProgress:
                        EndProgress();
                        break;
                    case ConfigurationSetState::Completed:
                        EndProgress();
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
            void HandleUnitProgress(const ConfigurationUnit& unit, ConfigurationUnitState state, const IConfigurationUnitResultInformation& resultInformation)
            {
                if (UnitHasPreviouslyCompleted(unit))
                {
                    return;
                }

                switch (state)
                {
                case ConfigurationUnitState::Pending:
                    // The unreported progress handler may send pending units, just ignore them
                    break;
                case ConfigurationUnitState::InProgress:
                    OutputUnitInProgressIfNeeded(unit);
                    BeginProgress();
                    break;
                case ConfigurationUnitState::Completed:
                    OutputUnitInProgressIfNeeded(unit);
                    EndProgress();
                    if (SUCCEEDED(resultInformation.ResultCode()))
                    {
                        m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationSuccessfullyApplied << std::endl;
                    }
                    else
                    {
                        OutputUnitRunFailure(m_context, unit, resultInformation);
                    }
                    MarkCompleted(unit);
                    OutputUnitCompletionProgress();
                    break;
                case ConfigurationUnitState::Skipped:
                    OutputUnitInProgressIfNeeded(unit);
                    AICLI_LOG(Config, Warning, << "Configuration unit " << Utility::ConvertToUTF8(unit.Type()) << "[" << Utility::ConvertToUTF8(unit.Identifier()) << "] was skipped with code 0x"
                        << Logging::SetHRFormat << resultInformation.ResultCode());
                    m_context.Reporter.Warn() << "  "_liv << GetUnitSkippedMessage(resultInformation) << std::endl;
                    MarkCompleted(unit);
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
                    OutputConfigurationUnitHeader(out, unit, unit.Details() ? unit.Details().UnitType() : unit.Type());
                }
            }

            std::set<winrt::guid> m_unitsSeen;
            bool m_isFirstProgress = true;
        };

        // Helper to handle progress callbacks from TestConfigurationSetAsync
        struct TestConfigurationSetProgressOutput final : public ConfigurationSetProgressOutputBase<TestConfigurationSetResult, TestConfigurationUnitResult>
        {
            using Operation = ConfigurationSetProgressOutputBase<TestConfigurationSetResult, TestConfigurationUnitResult>::Operation;

            TestConfigurationSetProgressOutput(Context& context, const Operation& operation) :
                ConfigurationSetProgressOutputBase(context, operation)
            {
                // Start the spinner for the first unit being tested since we only receive completions
                BeginProgress();
            }

            void Progress(const Operation& operation, const TestConfigurationUnitResult& data) override
            {
                auto threadContext = m_context.SetForCurrentThread();

                if (m_isFirstProgress)
                {
                    HandleUnreportedProgress(operation.GetResults());
                }

                HandleUnitProgress(data.Unit(), data.TestResult(), data.ResultInformation());
            }

            // If no progress has been reported, this function will report the given results
            void HandleUnreportedProgress(const TestConfigurationSetResult& result)
            {
                if (m_isFirstProgress)
                {
                    m_isFirstProgress = false;

                    for (const TestConfigurationUnitResult& unitResult : result.UnitResults())
                    {
                        HandleUnitProgress(unitResult.Unit(), unitResult.TestResult(), unitResult.ResultInformation());
                    }
                }
            }

        private:
            void HandleUnitProgress(const ConfigurationUnit& unit, ConfigurationTestResult testResult, const IConfigurationUnitResultInformation& resultInformation)
            {
                if (UnitHasPreviouslyCompleted(unit))
                {
                    return;
                }

                EndProgress();

                {
                    OutputStream info = m_context.Reporter.Info();
                    OutputConfigurationUnitHeader(info, unit, unit.Details() ? unit.Details().UnitType() : unit.Type());
                }

                switch (testResult)
                {
                case ConfigurationTestResult::Failed:
                    OutputUnitRunFailure(m_context, unit, resultInformation);
                    break;
                case ConfigurationTestResult::Negative:
                    m_context.Reporter.Warn() << "  "_liv << Resource::String::ConfigurationNotInDesiredState << std::endl;
                    break;
                case ConfigurationTestResult::NotRun:
                    m_context.Reporter.Warn() << "  "_liv << Resource::String::ConfigurationNoTestRun << std::endl;
                    break;
                case ConfigurationTestResult::Positive:
                    m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationInDesiredState << std::endl;
                    break;
                default: // ConfigurationTestResult::Unknown
                    m_context.Reporter.Error() << "  "_liv << Resource::String::ConfigurationUnexpectedTestResult(ToIntegral(testResult)) << std::endl;
                    break;
                }

                MarkCompleted(unit);
                OutputUnitCompletionProgress();
                BeginProgress();
            }

            bool m_isFirstProgress = true;
        };

        std::filesystem::path GetConfigurationFilePath(Execution::Context& context)
        {
            std::filesystem::path argPath = Utility::ConvertToUTF16(context.Args.GetArg(Args::Type::ConfigurationFile));
            return std::filesystem::weakly_canonical(argPath);
        }
    }

    void CreateConfigurationProcessor(Context& context)
    {
        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        progressScope->Callback().SetProgressMessage(Resource::String::ConfigurationInitializing());

        ConfigurationProcessor processor{ CreateConfigurationSetProcessorFactory(context)};

        // Set the processor to the current level of the logging.
        processor.MinimumLevel(ConvertLevel(Logging::Log().GetLevel()));
        processor.Caller(L"winget");
        // Use same activity as the overall winget command
        processor.ActivityIdentifier(*Logging::Telemetry().GetActivityId());
        // Apply winget telemetry setting to configuration
        processor.GenerateTelemetryEvents(!Settings::User().Get<Settings::Setting::TelemetryDisable>());

        // Route the configuration diagnostics into the context's diagnostics logging
        processor.Diagnostics([&context](const winrt::Windows::Foundation::IInspectable&, const IDiagnosticInformation& diagnostics)
            {
                context.GetThreadGlobals().GetDiagnosticLogger().Write(Logging::Channel::Config, ConvertLevel(diagnostics.Level()), Utility::ConvertToUTF8(diagnostics.Message()));
            });

        ConfigurationContext configurationContext;
        configurationContext.Processor(std::move(processor));

        context.Add<Data::ConfigurationContext>(std::move(configurationContext));
    }

    void OpenConfigurationSet(Context& context)
    {
        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        progressScope->Callback().SetProgressMessage(Resource::String::ConfigurationReadingConfigFile());

        std::filesystem::path absolutePath = GetConfigurationFilePath(context);

        Streams::IInputStream inputStream = nullptr;
        {
            auto openAction = Streams::FileRandomAccessStream::OpenAsync(absolutePath.wstring(), FileAccessMode::Read);
            auto cancellationScope = progressScope->Callback().SetCancellationFunction([&]() { openAction.Cancel(); });
            inputStream = openAction.get();
        }

        OpenConfigurationSetResult openResult = nullptr;
        {
            auto openAction = context.Get<Data::ConfigurationContext>().Processor().OpenConfigurationSetAsync(inputStream);
            auto cancellationScope = progressScope->Callback().SetCancellationFunction([&]() { openAction.Cancel(); });
            openResult = openAction.get();
        }

        progressScope.reset();

        if (FAILED_LOG(static_cast<HRESULT>(openResult.ResultCode().value)))
        {
            AICLI_LOG(Config, Error, << "Failed to open configuration set at " << absolutePath.u8string() << " with error 0x" << Logging::SetHRFormat << static_cast<HRESULT>(openResult.ResultCode().value));

            switch (openResult.ResultCode())
            {
            case WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE:
                context.Reporter.Error() << Resource::String::ConfigurationFieldInvalidType(Utility::LocIndString{ Utility::ConvertToUTF8(openResult.Field()) }) << std::endl;
                break;
            case WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE:
                context.Reporter.Error() << Resource::String::ConfigurationFieldInvalidValue(Utility::LocIndString{ Utility::ConvertToUTF8(openResult.Field()) }, Utility::LocIndString{ Utility::ConvertToUTF8(openResult.Value()) }) << std::endl;
                break;
            case WINGET_CONFIG_ERROR_MISSING_FIELD:
                context.Reporter.Error() << Resource::String::ConfigurationFieldMissing(Utility::LocIndString{ Utility::ConvertToUTF8(openResult.Field()) }) << std::endl;
                break;
            case WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION:
                context.Reporter.Error() << Resource::String::ConfigurationFileVersionUnknown(Utility::LocIndString{ Utility::ConvertToUTF8(openResult.Value()) }) << std::endl;
                break;
            case WINGET_CONFIG_ERROR_INVALID_CONFIGURATION_FILE:
            case WINGET_CONFIG_ERROR_INVALID_YAML:
            default:
                context.Reporter.Error() << Resource::String::ConfigurationFileInvalidYAML << std::endl;
                break;
            }

            if (openResult.Line() != 0)
            {
                context.Reporter.Error() << Resource::String::SeeLineAndColumn(openResult.Line(), openResult.Column()) << std::endl;
            }

            AICLI_TERMINATE_CONTEXT(openResult.ResultCode());
        }

        ConfigurationSet result = openResult.Set();

        // Fill out the information about the set based on it coming from a file.
        // TODO: Consider how to properly determine a good value for name and origin.
        result.Name(absolutePath.filename().wstring());
        result.Origin(absolutePath.parent_path().wstring());
        result.Path(absolutePath.wstring());

        context.Get<Data::ConfigurationContext>().Set(result);
    }

    void ShowConfigurationSet(Context& context)
    {
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();

        if (configContext.Set().Units().Size() == 0)
        {
            context.Reporter.Warn() << Resource::String::ConfigurationFileEmpty << std::endl;
            // This isn't an error termination, but there is no reason to proceed.
            AICLI_TERMINATE_CONTEXT(S_FALSE);
        }

        auto gettingDetailString = Resource::String::ConfigurationGettingDetails();
        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        progressScope->Callback().SetProgressMessage(gettingDetailString);

        auto getDetailsOperation = configContext.Processor().GetSetDetailsAsync(configContext.Set(), ConfigurationUnitDetailFlags::ReadOnly);
        auto unification = CreateProgressCancellationUnification(std::move(progressScope), getDetailsOperation);

        OutputStream out = context.Reporter.Info();
        uint32_t unitsShown = 0;

        getDetailsOperation.Progress([&](const IAsyncOperationWithProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult>& operation, const GetConfigurationUnitDetailsResult&)
            {
                auto threadContext = context.SetForCurrentThread();

                unification.Reset();

                auto unitResults = operation.GetResults().UnitResults();
                for (unitsShown; unitsShown < unitResults.Size(); ++unitsShown)
                {
                    GetConfigurationUnitDetailsResult unitResult = unitResults.GetAt(unitsShown);
                    LogFailedGetConfigurationUnitDetails(unitResult.Unit(), unitResult.ResultInformation());
                    OutputConfigurationUnitInformation(out, unitResult.Unit());
                }

                progressScope = context.Reporter.BeginAsyncProgress(true);
                progressScope->Callback().SetProgressMessage(gettingDetailString);
                unification.Progress(std::move(progressScope));
            });

        HRESULT hr = S_OK;
        GetConfigurationSetDetailsResult result = nullptr;

        try
        {
            result = getDetailsOperation.get();
        }
        catch (...)
        {
            hr = LOG_CAUGHT_EXCEPTION();
        }

        unification.Reset();

        if (context.IsTerminated())
        {
            // The context should only be terminated on us due to cancellation
            context.Reporter.Error() << Resource::String::Cancelled << std::endl;
            return;
        }

        if (FAILED(hr))
        {
            // Failing to get details might not be fatal, warn about it but proceed
            context.Reporter.Warn() << Resource::String::ConfigurationFailedToGetDetails << std::endl;
        }

        // Handle any missing progress callbacks that are in the results
        if (result)
        {
            auto unitResults = result.UnitResults();
            if (unitResults)
            {
                for (unitsShown; unitsShown < unitResults.Size(); ++unitsShown)
                {
                    GetConfigurationUnitDetailsResult unitResult = unitResults.GetAt(unitsShown);
                    LogFailedGetConfigurationUnitDetails(unitResult.Unit(), unitResult.ResultInformation());
                    OutputConfigurationUnitInformation(out, unitResult.Unit());
                }
            }
        }

        // Handle any units that are NOT in the results (due to an exception part of the way through)
        auto allUnits = configContext.Set().Units();
        for (unitsShown; unitsShown < allUnits.Size(); ++unitsShown)
        {
            ConfigurationUnit unit = allUnits.GetAt(unitsShown);
            OutputConfigurationUnitInformation(out, unit);
        }
    }

    void ShowConfigurationSetConflicts(Execution::Context& context)
    {
        UNREFERENCED_PARAMETER(context);
    }

    void ConfirmConfigurationProcessing::operator()(Execution::Context& context) const
    {
        context.Reporter.Warn() << Resource::String::ConfigurationWarning << std::endl;

        if (!context.Args.Contains(Args::Type::ConfigurationAcceptWarning))
        {
            context << RequireInteractivity(WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED);
            if (context.IsTerminated())
            {
                return;
            }

            auto promptString = m_isApply ? Resource::String::ConfigurationWarningPromptApply : Resource::String::ConfigurationWarningPromptTest;
            if (!context.Reporter.PromptForBoolResponse(promptString, Reporter::Level::Warning))
            {
                AICLI_TERMINATE_CONTEXT(WINGET_CONFIG_ERROR_WARNING_NOT_ACCEPTED);
            }
        }
    }

    void ApplyConfigurationSet(Execution::Context& context)
    {
        ApplyConfigurationSetResult result = nullptr;
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();

        {
            auto applyOperation = configContext.Processor().ApplySetAsync(configContext.Set(), ApplyConfigurationSetFlags::None);
            ApplyConfigurationSetProgressOutput progress{ context, applyOperation };

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

    void TestConfigurationSet(Execution::Context& context)
    {
        TestConfigurationSetResult result = nullptr;
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();

        {
            auto testOperation = configContext.Processor().TestSetAsync(configContext.Set());
            TestConfigurationSetProgressOutput progress{ context, testOperation };

            result = testOperation.get();
            progress.HandleUnreportedProgress(result);
        }

        switch (result.TestResult())
        {
        case ConfigurationTestResult::Failed:
            context.Reporter.Error() << Resource::String::ConfigurationFailedToTest << std::endl;
            AICLI_TERMINATE_CONTEXT(WINGET_CONFIG_ERROR_TEST_FAILED);
            break;
        case ConfigurationTestResult::Negative:
            context.Reporter.Warn() << Resource::String::ConfigurationNotInDesiredState << std::endl;
            context.SetTerminationHR(S_FALSE);
            break;
        case ConfigurationTestResult::NotRun:
            context.Reporter.Warn() << Resource::String::ConfigurationNoTestRun << std::endl;
            AICLI_TERMINATE_CONTEXT(WINGET_CONFIG_ERROR_TEST_NOT_RUN);
            break;
        case ConfigurationTestResult::Positive:
            context.Reporter.Info() << Resource::String::ConfigurationInDesiredState << std::endl;
            break;
        default: // ConfigurationTestResult::Unknown
            context.Reporter.Error() << Resource::String::ConfigurationUnexpectedTestResult(ToIntegral(result.TestResult())) << std::endl;
            AICLI_TERMINATE_CONTEXT(E_FAIL);
            break;
        }
    }

    void VerifyIsFullPackage(Execution::Context& context)
    {
        if (IsStubPackage())
        {
            context.Reporter.Error() << Resource::String::ConfigurationNotEnabledMessage << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PACKAGE_IS_STUB);
        }
    }

    void ValidateConfigurationSetSemantics(Execution::Context& context)
    {
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();

        if (configContext.Set().Units().Size() == 0)
        {
            context.Reporter.Warn() << Resource::String::ConfigurationFileEmpty << std::endl;
            // This isn't an error termination, but there is no reason to proceed.
            AICLI_TERMINATE_CONTEXT(S_FALSE);
        }

        ApplyConfigurationSetResult result = configContext.Processor().ApplySet(configContext.Set(), ApplyConfigurationSetFlags::PerformConsistencyCheckOnly);

        if (FAILED(result.ResultCode()))
        {
            for (const auto& unitResult : result.UnitResults())
            {
                IConfigurationUnitResultInformation resultInformation = unitResult.ResultInformation();
                winrt::hresult resultCode = resultInformation.ResultCode();

                if (FAILED(resultCode))
                {
                    ConfigurationUnit unit = unitResult.Unit();

                    auto out = context.Reporter.Info();
                    OutputConfigurationUnitHeader(out, unit, unit.Type());

                    switch (resultCode)
                    {
                    case WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER:
                        context.Reporter.Error() << "  "_liv << Resource::String::ConfigurationUnitHasDuplicateIdentifier(Utility::LocIndString{ Utility::ConvertToUTF8(unit.Identifier()) }) << std::endl;
                        break;
                    case WINGET_CONFIG_ERROR_MISSING_DEPENDENCY:
                        context.Reporter.Error() << "  "_liv << Resource::String::ConfigurationUnitHasMissingDependency(Utility::LocIndString{ Utility::ConvertToUTF8(resultInformation.Details()) }) << std::endl;
                        break;
                    case WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED:
                        context.Reporter.Error() << "  "_liv << Resource::String::ConfigurationUnitIsPartOfDependencyCycle << std::endl;
                        break;
                    default:
                        context.Reporter.Error() << "  "_liv << Resource::String::ConfigurationUnitFailed(static_cast<int32_t>(resultCode)) << std::endl;
                        break;
                    }
                }
            }

            AICLI_TERMINATE_CONTEXT(result.ResultCode());
        }
    }

    void ValidateConfigurationSetUnitProcessors(Execution::Context& context)
    {
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();

        // TODO: We could optimize this by creating a set with unique resource units

        // First get the local details
        auto gettingDetailString = Resource::String::ConfigurationGettingDetails();
        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        progressScope->Callback().SetProgressMessage(gettingDetailString);

        auto getLocalDetailsOperation = configContext.Processor().GetSetDetailsAsync(configContext.Set(), ConfigurationUnitDetailFlags::Local);
        auto unification = CreateProgressCancellationUnification(std::move(progressScope), getLocalDetailsOperation);

        HRESULT getLocalHR = S_OK;
        GetConfigurationSetDetailsResult getLocalResult = nullptr;

        try
        {
            getLocalResult = getLocalDetailsOperation.get();
        }
        catch (...)
        {
            getLocalHR = LOG_CAUGHT_EXCEPTION();
        }

        unification.Reset();

        if (context.IsTerminated())
        {
            // The context should only be terminated on us due to cancellation
            context.Reporter.Error() << Resource::String::Cancelled << std::endl;
            return;
        }

        if (FAILED(getLocalHR))
        {
            // Failing to get details might not be fatal, warn about it but proceed
            context.Reporter.Warn() << Resource::String::ConfigurationFailedToGetDetails << std::endl;
        }

        // Next get the details from the catalog
        progressScope = context.Reporter.BeginAsyncProgress(true);
        progressScope->Callback().SetProgressMessage(gettingDetailString);

        auto getCatalogDetailsOperation = configContext.Processor().GetSetDetailsAsync(configContext.Set(), ConfigurationUnitDetailFlags::Catalog);
        unification = CreateProgressCancellationUnification(std::move(progressScope), getCatalogDetailsOperation);

        HRESULT getCatalogHR = S_OK;
        GetConfigurationSetDetailsResult getCatalogResult = nullptr;

        try
        {
            getCatalogResult = getCatalogDetailsOperation.get();
        }
        catch (...)
        {
            getCatalogHR = LOG_CAUGHT_EXCEPTION();
        }

        unification.Reset();

        if (context.IsTerminated())
        {
            // The context should only be terminated on us due to cancellation
            context.Reporter.Error() << Resource::String::Cancelled << std::endl;
            return;
        }

        if (FAILED(getCatalogHR))
        {
            // Failing to get the catalog details means that we can't really get give much of a meaningful response.
            context.Reporter.Error() << Resource::String::ConfigurationFailedToGetDetails << std::endl;
            AICLI_TERMINATE_CONTEXT(getCatalogHR);
        }

        auto units = configContext.Set().Units();
        auto localUnitResults = getLocalResult ? getLocalResult.UnitResults() : nullptr;
        if (localUnitResults && units.Size() != localUnitResults.Size())
        {
            AICLI_LOG(Config, Error, << "The details result size did not match the set size: Set[" << units.Size() << "], Local[" << localUnitResults.Size() << "]");
            THROW_HR(WINGET_CONFIG_ERROR_ASSERTION_FAILED);
        }

        auto catalogUnitResults = getCatalogResult.UnitResults();
        if (units.Size() != catalogUnitResults.Size())
        {
            AICLI_LOG(Config, Error, << "The details result sizes did not match the set size: Set[" << units.Size() << "], Catalog[" << catalogUnitResults.Size() << "]");
            THROW_HR(WINGET_CONFIG_ERROR_ASSERTION_FAILED);
        }

        bool foundIssue = false;

        // Now that we have the entire set of local and catalog details, process each unit
        for (uint32_t i = 0; i < units.Size(); ++i)
        {
            const ConfigurationUnit& unit = units.GetAt(i);
            GetConfigurationUnitDetailsResult localUnitResult = localUnitResults ? localUnitResults.GetAt(i) : nullptr;
            GetConfigurationUnitDetailsResult catalogUnitResult = catalogUnitResults.GetAt(i);
            IConfigurationUnitProcessorDetails catalogDetails = catalogUnitResult.Details();

            bool needsHeader = true;
            auto outputHeaderIfNeeded = [&]()
            {
                if (needsHeader)
                {
                    auto out = context.Reporter.Info();
                    OutputConfigurationUnitHeader(out, unit, unit.Type());
                    needsHeader = false;
                    foundIssue = true;
                }
            };

            if (GetValueSetString(unit.Metadata(), s_Directive_Module).value_or(Utility::LocIndString{}).empty())
            {
                outputHeaderIfNeeded();
                context.Reporter.Warn() << "  "_liv << Resource::String::ConfigurationUnitModuleNotProvidedWarning << std::endl;
            }

            if (catalogDetails)
            {
                // Warn if unit is not public
                if (!catalogDetails.IsPublic())
                {
                    outputHeaderIfNeeded();
                    context.Reporter.Warn() << "  "_liv << Resource::String::ConfigurationUnitNotPublicWarning << std::endl;
                }

                // Since it is available, no more checks are needed
                continue;
            }
            // Everything below here is due to not finding in the catalog search

            if (FAILED(catalogUnitResult.ResultInformation().ResultCode()))
            {
                outputHeaderIfNeeded();
                OutputUnitRunFailure(context, unit, catalogUnitResult.ResultInformation());
                continue;
            }

            // If not already prerelease, try with prerelease and warn if found
            std::optional<bool> allowPrereleaseDirective = GetValueSetBool(unit.Metadata(), s_Directive_AllowPrerelease);
            if (!allowPrereleaseDirective || !allowPrereleaseDirective.value())
            {
                // Check if the configuration unit is prerelease but the author forgot it
                ConfigurationUnit clone = unit.Copy();
                clone.Metadata().Insert(s_Directive_AllowPrerelease, PropertyValue::CreateBoolean(true));

                progressScope = context.Reporter.BeginAsyncProgress(true);
                progressScope->Callback().SetProgressMessage(gettingDetailString);

                auto getUnitDetailsOperation = configContext.Processor().GetUnitDetailsAsync(clone, ConfigurationUnitDetailFlags::Catalog);
                auto unitUnification = CreateProgressCancellationUnification(std::move(progressScope), getUnitDetailsOperation);

                IConfigurationUnitProcessorDetails prereleaseDetails;

                try
                {
                    prereleaseDetails = getUnitDetailsOperation.get().Details();
                }
                CATCH_LOG();

                unification.Reset();

                if (prereleaseDetails)
                {
                    outputHeaderIfNeeded();
                    context.Reporter.Warn() << "  "_liv << Resource::String::ConfigurationUnitNeedsPrereleaseWarning << std::endl;
                    continue;
                }
            }

            // If module is local, warn that we couldn't find it in the catalog
            if (localUnitResult && localUnitResult.Details())
            {
                outputHeaderIfNeeded();
                context.Reporter.Warn() << "  "_liv << Resource::String::ConfigurationUnitNotInCatalogWarning << std::endl;
                continue;
            }

            // Finally, error that we couldn't find it at all
            outputHeaderIfNeeded();
            context.Reporter.Error() << "  "_liv << Resource::String::ConfigurationUnitNotFound << std::endl;
        }

        if (foundIssue)
        {
            // Indicate that it was not a total success
            AICLI_TERMINATE_CONTEXT(S_FALSE);
        }
    }

    void ValidateAllGoodMessage(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::ConfigurationValidationFoundNoIssues << std::endl;
    }
}
