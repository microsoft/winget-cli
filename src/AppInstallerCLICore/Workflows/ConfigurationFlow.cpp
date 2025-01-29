// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationFlow.h"
#include "ImportExportFlow.h"
#include "PromptFlow.h"
#include "TableOutput.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include "ConfigurationCommon.h"
#include "ConfigurationWingetDscModuleUnitValidation.h"
#include <AppInstallerDateTime.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerErrors.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>
#include <winget/ExperimentalFeature.h>
#include <winget/SelfManagement.h>
#include <winrt/Microsoft.Management.Configuration.h>

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

    namespace anon
    {
        constexpr std::wstring_view s_Directive_Description = L"description";
        constexpr std::wstring_view s_Directive_Module = L"module";
        constexpr std::wstring_view s_Directive_AllowPrerelease = L"allowPrerelease";

        constexpr std::wstring_view s_Unit_WinGetPackage = L"WinGetPackage";
        constexpr std::wstring_view s_Unit_WinGetSource = L"WinGetSource";

        constexpr std::wstring_view s_Module_WinGetClient = L"Microsoft.WinGet.DSC";

        constexpr std::wstring_view s_Setting_WinGetPackage_Id = L"id";
        constexpr std::wstring_view s_Setting_WinGetPackage_Source = L"source";
        constexpr std::wstring_view s_Setting_WinGetPackage_Version = L"version";

        constexpr std::wstring_view s_Setting_WinGetSource_Name = L"name";
        constexpr std::wstring_view s_Setting_WinGetSource_Arg = L"argument";
        constexpr std::wstring_view s_Setting_WinGetSource_Type = L"type";

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

            IConfigurationSetProcessorFactory factory;

            // Since downgrading is not currently supported, only use dynamic if running limited.
            if (Runtime::IsRunningWithLimitedToken())
            {
                factory = ConfigurationRemoting::CreateDynamicRuntimeFactory();
            }
            else
            {
                factory = ConfigurationRemoting::CreateOutOfProcessFactory();
            }

            Configuration::SetModulePath(context, factory);
            return factory;
        }

        void ConfigureProcessorForUse(Execution::Context& context, ConfigurationProcessor&& processor)
        {
            // Set the processor to the current level of the logging.
            processor.MinimumLevel(anon::ConvertLevel(Logging::Log().GetLevel()));
            processor.Caller(L"winget");
            // Use same activity as the overall winget command
            processor.ActivityIdentifier(*Logging::Telemetry().GetActivityId());
            // Apply winget telemetry setting to configuration
            processor.GenerateTelemetryEvents(!Settings::User().Get<Settings::Setting::TelemetryDisable>());

            // Route the configuration diagnostics into the context's diagnostics logging
            processor.Diagnostics([&context](const winrt::Windows::Foundation::IInspectable&, const IDiagnosticInformation& diagnostics)
                {
                    context.GetThreadGlobals().GetDiagnosticLogger().Write(Logging::Channel::Config, anon::ConvertLevel(diagnostics.Level()), Utility::ConvertToUTF8(diagnostics.Message()));
                });

            ConfigurationContext configurationContext;
            configurationContext.Processor(std::move(processor));

            context.Add<Data::ConfigurationContext>(std::move(configurationContext));
        }

        winrt::hstring GetValueSetString(const ValueSet& valueSet, std::wstring_view value)
        {
            if (valueSet.HasKey(value))
            {
                auto object = valueSet.Lookup(value);
                IPropertyValue property = object.try_as<IPropertyValue>();
                if (property && property.Type() == PropertyType::String)
                {
                    return property.GetString();
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

        // Contains the output functions and tracks whether any fields needed to be truncated.
        struct OutputHelper
        {
            OutputHelper(Execution::Context& context) : m_context(context) {}

            size_t ValuesTruncated = 0;

            // Converts a string from the configuration API surface for output.
            // All strings coming from the API are external data and not localizable by us.
            Utility::LocIndString ConvertForOutput(const std::string& input, size_t maxLines)
            {
                bool truncated = false;
                auto lines = Utility::SplitIntoLines(input);

                if (maxLines == 1 && lines.size() > 1)
                {
                    // If the limit was one line, don't allow line breaks but do allow a second line of overflow
                    lines.resize(1);
                    maxLines = 2;
                    truncated = true;
                }

                if (Utility::LimitOutputLines(lines, GetConsoleWidth(), maxLines))
                {
                    truncated = true;
                }

                if (truncated)
                {
                    ++ValuesTruncated;
                }

                return Utility::LocIndString{ Utility::Join("\n", lines) };
            }

            Utility::LocIndString ConvertForOutput(const winrt::hstring& input, size_t maxLines)
            {
                return ConvertForOutput(Utility::ConvertToUTF8(input), maxLines);
            }

            Utility::LocIndString ConvertIdentifier(const winrt::hstring& input)
            {
                return ConvertForOutput(input, 1);
            }

            Utility::LocIndString ConvertURI(const winrt::hstring& input)
            {
                return ConvertForOutput(input, 1);
            }

            Utility::LocIndString ConvertValue(const winrt::hstring& input)
            {
                return ConvertForOutput(input, 5);
            }

            Utility::LocIndString ConvertDetailsIdentifier(const winrt::hstring& input)
            {
                return ConvertForOutput(Utility::ConvertControlCodesToPictures(Utility::ConvertToUTF8(input)), 1);
            }

            Utility::LocIndString ConvertDetailsURI(const winrt::hstring& input)
            {
                return ConvertForOutput(Utility::ConvertControlCodesToPictures(Utility::ConvertToUTF8(input)), 1);
            }

            Utility::LocIndString ConvertDetailsValue(const winrt::hstring& input)
            {
                return ConvertForOutput(Utility::ConvertControlCodesToPictures(Utility::ConvertToUTF8(input)), 5);
            }

            void OutputValueWithTruncationWarningIfNeeded(const winrt::hstring& input)
            {
                size_t truncatedBefore = ValuesTruncated;
                m_context.Reporter.Info() << ConvertValue(input) << '\n';

                if (ValuesTruncated > truncatedBefore)
                {
                    m_context.Reporter.Warn() << Resource::String::ConfigurationWarningValueTruncated << std::endl;
                }
            }

            void OutputPropertyValue(const IPropertyValue property)
            {
                switch (property.Type())
                {
                case PropertyType::String:
                    m_context.Reporter.Info() << ' ';
                    OutputValueWithTruncationWarningIfNeeded(property.GetString());
                    break;
                case PropertyType::Boolean:
                    m_context.Reporter.Info() << ' ' << (property.GetBoolean() ? Utility::LocIndView("true") : Utility::LocIndView("false")) << '\n';
                    break;
                case PropertyType::Int64:
                    m_context.Reporter.Info() << ' ' << property.GetInt64() << '\n';
                    break;
                default:
                    m_context.Reporter.Info() << " [Debug:PropertyType="_liv << property.Type() << "]\n"_liv;
                    break;
                }
            }

            void OutputValueSetAsArray(const ValueSet& valueSetArray, size_t indent)
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

                    m_context.Reporter.Info() << indentString << "-";
                    if (arrayProperty)
                    {
                        OutputPropertyValue(arrayProperty);
                    }
                    else
                    {
                        ValueSet arraySubset = arrayObject.as<ValueSet>();
                        auto size = arraySubset.Size();
                        if (size > 0)
                        {
                            // First one is special.
                            auto first = arraySubset.First().Current();
                            m_context.Reporter.Info() << ' ' << ConvertIdentifier(first.Key()) << ':';

                            auto object = first.Value();
                            IPropertyValue property = object.try_as<IPropertyValue>();
                            if (property)
                            {
                                OutputPropertyValue(property);
                            }
                            else
                            {
                                // If not an IPropertyValue, it must be a ValueSet
                                ValueSet subset = object.as<ValueSet>();
                                m_context.Reporter.Info() << '\n';
                                OutputValueSet(subset, indent + 4);
                            }

                            if (size > 1)
                            {
                                arraySubset.Remove(first.Key());
                                OutputValueSet(arraySubset, indent + 2);
                                arraySubset.Insert(first.Key(), first.Value());
                            }
                        }
                    }
                }
            }

            void OutputValueSet(const ValueSet& valueSet, size_t indent)
            {
                Utility::LocIndString indentString{ std::string(indent, ' ') };

                for (const auto& value : valueSet)
                {
                    m_context.Reporter.Info() << indentString << ConvertIdentifier(value.Key()) << ':';

                    auto object = value.Value();

                    IPropertyValue property = object.try_as<IPropertyValue>();
                    if (property)
                    {
                        OutputPropertyValue(property);
                    }
                    else
                    {
                        // If not an IPropertyValue, it must be a ValueSet
                        ValueSet subset = object.as<ValueSet>();
                        m_context.Reporter.Info() << '\n';
                        if (subset.HasKey(L"treatAsArray"))
                        {
                            OutputValueSetAsArray(subset, indent + 2);
                        }
                        else
                        {
                            OutputValueSet(subset, indent + 2);
                        }
                    }
                }
            }

            void OutputConfigurationUnitHeader(const ConfigurationUnit& unit, const winrt::hstring& name)
            {
                m_context.Reporter.Info() << ConfigurationIntentEmphasis << ToResource(unit.Intent()) << " :: "_liv << ConfigurationUnitEmphasis << ConvertIdentifier(name);

                winrt::hstring identifier = unit.Identifier();
                if (!identifier.empty())
                {
                    m_context.Reporter.Info() << " ["_liv << ConvertIdentifier(identifier) << ']';
                }

                m_context.Reporter.Info() << '\n';
            }

            void OutputConfigurationUnitInformation(const ConfigurationUnit& unit)
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
                    OutputConfigurationUnitHeader(unit, details.UnitType());

                    auto unitDocumentationUri = details.UnitDocumentationUri();
                    if (unitDocumentationUri)
                    {
                        m_context.Reporter.Info() << "  "_liv << ConvertDetailsURI(unitDocumentationUri.DisplayUri()) << '\n';
                    }

                    winrt::hstring unitDescriptionFromDetails = details.UnitDescription();
                    if (!unitDescriptionFromDetails.empty())
                    {
                        m_context.Reporter.Info() << "  "_liv << ConvertDetailsValue(unitDescriptionFromDetails) << '\n';
                    }
                    else
                    {
                        auto unitDescriptionFromDirectives = GetValueSetString(metadata, s_Directive_Description);
                        if (!unitDescriptionFromDirectives.empty())
                        {
                            m_context.Reporter.Info() << "  "_liv;
                            OutputValueWithTruncationWarningIfNeeded(unitDescriptionFromDirectives);
                        }
                    }

                    auto author = ConvertDetailsIdentifier(details.Author());
                    if (author.empty())
                    {
                        author = ConvertDetailsIdentifier(details.Publisher());
                    }
                    if (details.IsLocal())
                    {
                        m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationModuleWithDetails(ConvertDetailsIdentifier(details.ModuleName()), author, Resource::String::ConfigurationLocal) << '\n';
                    }
                    else
                    {
                        m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationModuleWithDetails(ConvertDetailsIdentifier(details.ModuleName()), author, ConvertDetailsIdentifier(details.ModuleSource())) << '\n';
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
                        m_context.Reporter.Info() << "    "_liv << ConvertDetailsURI(moduleUri.DisplayUri()) << '\n';
                    }

                    winrt::hstring moduleDescription = details.ModuleDescription();
                    if (!moduleDescription.empty())
                    {
                        m_context.Reporter.Info() << "    "_liv << ConvertDetailsValue(moduleDescription) << '\n';
                    }
                }
                else
                {
                    // -- Sample output when no IConfigurationUnitProcessorDetails present --
                    // Intent :: Type <from unit> [identifier]
                    //   Description (from directives)
                    //   "Module": module <directive>
                    OutputConfigurationUnitHeader(unit, unit.Type());

                    auto description = GetValueSetString(metadata, s_Directive_Description);
                    if (!description.empty())
                    {
                        m_context.Reporter.Info() << "  "_liv;
                        OutputValueWithTruncationWarningIfNeeded(description);
                    }

                    auto module = GetValueSetString(metadata, s_Directive_Module);
                    if (!module.empty())
                    {
                        m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationModuleNameOnly(ConvertIdentifier(module)) << '\n';
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
                        allDependencies << ' ' << ConvertIdentifier(dependency);
                    }
                    m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationDependencies(Utility::LocIndString{ std::move(allDependencies).str() }) << '\n';
                }

                ValueSet settings = unit.Settings();
                if (settings.Size() > 0)
                {
                    m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationSettings << '\n';
                    OutputValueSet(settings, 4);
                }
            }

        private:
            Execution::Context& m_context;
        };

        void OutputConfigurationUnitHeader(Execution::Context& context, const ConfigurationUnit& unit, const winrt::hstring& name)
        {
            OutputHelper helper{ context };
            helper.OutputConfigurationUnitHeader(unit, name);
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
            case WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE: return { Resource::String::ConfigurationUnitModuleImportFailed(), false };
            case WINGET_CONFIG_ERROR_UNIT_INVOKE_INVALID_RESULT: return { Resource::String::ConfigurationUnitReturnedInvalidResult(), false };
            case WINGET_CONFIG_ERROR_UNIT_SETTING_CONFIG_ROOT: return { Resource::String::ConfigurationUnitSettingConfigRoot(), false };
            case WINGET_CONFIG_ERROR_UNIT_IMPORT_MODULE_ADMIN: return { Resource::String::ConfigurationUnitImportModuleAdmin(), false };
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
                        m_context.Reporter.Info() << "  "_liv << Resource::String::ConfigurationUnitSuccessfullyApplied << std::endl;
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

                    OutputConfigurationUnitHeader(m_context, unit, unit.Details() ? unit.Details().UnitType() : unit.Type());
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

                OutputConfigurationUnitHeader(m_context, unit, unit.Details() ? unit.Details().UnitType() : unit.Type());

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

        std::string GetNormalizedIdentifier(const winrt::hstring& identifier)
        {
            return Utility::FoldCase(Utility::NormalizedString{ identifier });
        }

        // Get unit validation order. Make sure dependency units are before units depending on them.
        std::vector<uint32_t> GetConfigurationSetUnitValidationOrder(winrt::Windows::Foundation::Collections::IVectorView<ConfigurationUnit> units)
        {
            // Create id to index map for easier processing.
            std::map<std::string, uint32_t> idToUnitIndex;
            for (uint32_t i = 0; i < units.Size(); ++i)
            {
                auto id = GetNormalizedIdentifier(units.GetAt(i).Identifier());
                if (!id.empty())
                {
                    idToUnitIndex.emplace(std::move(id), i);
                }
            }

            // We do not need to worry about duplicate id, missing dependency or loops
            // since dependency integrity is already validated in earlier semantic checks.

            std::vector<uint32_t> validationOrder;

            std::function<void(const ConfigurationUnit&, uint32_t)> addUnitToValidationOrder =
                [&](const ConfigurationUnit& unit, uint32_t index)
                {
                    if (std::find(validationOrder.begin(), validationOrder.end(), index) == validationOrder.end())
                    {
                        for (auto const& dependencyId : unit.Dependencies())
                        {
                            auto dependencyIndex = idToUnitIndex.find(GetNormalizedIdentifier(dependencyId))->second;
                            addUnitToValidationOrder(units.GetAt(dependencyIndex), dependencyIndex);
                        }
                        validationOrder.emplace_back(index);
                    }
                };

            for (uint32_t i = 0; i < units.Size(); ++i)
            {
                addUnitToValidationOrder(units.GetAt(i), i);
            }

            THROW_HR_IF(E_UNEXPECTED, units.Size() != validationOrder.size());

            return validationOrder;
        }

        void SetNameAndOrigin(ConfigurationSet& set, std::filesystem::path& absolutePath)
        {
            // TODO: Consider how to properly determine a good value for name and origin.
            set.Name(absolutePath.filename().wstring());
            set.Origin(absolutePath.parent_path().wstring());
            set.Path(absolutePath.wstring());
        }

        void OpenConfigurationSet(Execution::Context& context, const std::string& argPath, bool allowRemote)
        {
            auto progressScope = context.Reporter.BeginAsyncProgress(true);
            progressScope->Callback().SetProgressMessage(Resource::String::ConfigurationReadingConfigFile());

            std::wstring argPathWide = Utility::ConvertToUTF16(argPath);
            bool isRemote = Utility::IsUrlRemote(argPath);
            std::filesystem::path absolutePath;
            Streams::IInputStream inputStream = nullptr;

            if (isRemote)
            {
                if (!allowRemote)
                {
                    AICLI_LOG(Config, Error, << "Remote files are not supported");
                    AICLI_TERMINATE_CONTEXT(ERROR_NOT_SUPPORTED);
                }

                std::ostringstream stringStream;
                ProgressCallback emptyCallback;
                Utility::DownloadToStream(argPath, stringStream, Utility::DownloadType::ConfigurationFile, emptyCallback);

                auto strContent = stringStream.str();
                std::vector<BYTE> byteContent{ strContent.begin(), strContent.end() };

                Streams::InMemoryRandomAccessStream memoryStream;
                Streams::DataWriter streamWriter{ memoryStream };
                streamWriter.WriteBytes(byteContent);
                streamWriter.StoreAsync().get();
                streamWriter.DetachStream();
                memoryStream.Seek(0);
                inputStream = memoryStream;
            }
            else
            {
                absolutePath = std::filesystem::weakly_canonical(std::filesystem::path{ argPathWide });
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
                AICLI_LOG(Config, Error, << "Failed to open configuration set at " << (isRemote ? argPath : absolutePath.u8string()) << " with error 0x" << Logging::SetHRFormat << static_cast<HRESULT>(openResult.ResultCode().value));

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

            // Temporary block on using schema 0.3 while experimental
            if (result.SchemaVersion() == L"0.3")
            {
                AICLI_RETURN_IF_TERMINATED(context << EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature::Configuration03));
            }

            // Fill out the information about the set based on it coming from a file.
            if (isRemote)
            {
                result.Name(Utility::GetFileNameFromURI(argPath).wstring());
                result.Origin(argPathWide);
                // Do not set path. This means ${WinGetConfigRoot} not supported in remote configs.
            }
            else
            {
                SetNameAndOrigin(result, absolutePath);
            }

            context.Get<Data::ConfigurationContext>().Set(result);
        }

        ConfigurationUnit CreateWinGetSourceUnit(const PackageCollection::Source& source)
        {
            std::string sourceUnitId = source.Details.Name + '_' + source.Details.Type;
            std::wstring sourceUnitIdWide = Utility::ConvertToUTF16(sourceUnitId);

            ConfigurationUnit unit;
            unit.Type(s_Unit_WinGetSource);
            unit.Identifier(sourceUnitIdWide);
            unit.Intent(ConfigurationUnitIntent::Apply);

            auto description = Resource::String::ConfigureExportUnitDescription(Utility::LocIndView{ sourceUnitId });

            ValueSet directives;
            directives.Insert(s_Directive_Module, PropertyValue::CreateString(s_Module_WinGetClient));
            directives.Insert(s_Directive_Description, PropertyValue::CreateString(winrt::to_hstring(description.get())));
            unit.Metadata(directives);

            ValueSet settings;
            settings.Insert(s_Setting_WinGetSource_Name, PropertyValue::CreateString(Utility::ConvertToUTF16(source.Details.Name)));
            settings.Insert(s_Setting_WinGetSource_Arg, PropertyValue::CreateString(Utility::ConvertToUTF16(source.Details.Arg)));
            settings.Insert(s_Setting_WinGetSource_Type, PropertyValue::CreateString(Utility::ConvertToUTF16(source.Details.Type)));
            unit.Settings(settings);

            unit.Environment().Context(SecurityContext::Elevated);

            return unit;
        }

        ConfigurationUnit CreateWinGetPackageUnit(const PackageCollection::Package& package, const PackageCollection::Source& source, bool includeVersion, const std::optional<ConfigurationUnit>& dependentUnit)
        {
            std::wstring packageIdWide = Utility::ConvertToUTF16(package.Id);
            std::wstring sourceNameWide = Utility::ConvertToUTF16(source.Details.Name);

            ConfigurationUnit unit;
            unit.Type(s_Unit_WinGetPackage);
            unit.Identifier(sourceNameWide + L'_' + packageIdWide);
            unit.Intent(ConfigurationUnitIntent::Apply);

            auto description = Resource::String::ConfigureExportUnitInstallDescription(Utility::LocIndView{ package.Id });

            ValueSet directives;
            directives.Insert(s_Directive_Module, PropertyValue::CreateString(s_Module_WinGetClient));
            directives.Insert(s_Directive_Description, PropertyValue::CreateString(winrt::to_hstring(description.get())));
            unit.Metadata(directives);

            ValueSet settings;
            settings.Insert(s_Setting_WinGetPackage_Id, PropertyValue::CreateString(packageIdWide));
            settings.Insert(s_Setting_WinGetPackage_Source, PropertyValue::CreateString(sourceNameWide));
            if (includeVersion)
            {
                settings.Insert(s_Setting_WinGetPackage_Version, PropertyValue::CreateString(Utility::ConvertToUTF16(package.VersionAndChannel.GetVersion().ToString())));
            }
            unit.Settings(settings);

            // TODO: We may consider setting security environment based on installer elevation requirements?

            // Add dependency if needed.
            if (dependentUnit.has_value())
            {
                auto dependencies = winrt::single_threaded_vector<winrt::hstring>();
                dependencies.Append(dependentUnit.value().Identifier());
                unit.Dependencies(std::move(dependencies));
            }

            return unit;
        }

        GetConfigurationUnitSettingsResult GetUnitSettings(Execution::Context& context, ConfigurationUnit& unit)
        {
            // This assumes there are no required properties for Get, but for example WinGetPackage requires the Id.
            // It is obviously wrong and will be wrong until Export is implemented for DSC v2 and a proper way to inform
            // about input to winget configure export is implemented. Drink the kool-aid and transcend.
            unit.Intent(ConfigurationUnitIntent::Inform);

            auto progressScope = context.Reporter.BeginAsyncProgress(true);

            progressScope->Callback().SetProgressMessage(Resource::String::ConfigurationGettingResourceSettings());

            GetConfigurationUnitSettingsResult getResult = nullptr;
            {
                auto getAction = context.Get<Data::ConfigurationContext>().Processor().GetUnitSettingsAsync(unit);
                auto cancellationScope = progressScope->Callback().SetCancellationFunction([&]() { getAction.Cancel(); });
                getResult = getAction.get();
            }

            progressScope.reset();
            return getResult;
        }

        ConfigurationUnit CreateConfigurationUnit(Execution::Context& context, std::string_view moduleName, std::string_view resourceName, const std::optional<ConfigurationUnit>& dependentUnit)
        {
            std::wstring moduleNameWide = Utility::ConvertToUTF16(moduleName);
            std::wstring resourceNameWide = Utility::ConvertToUTF16(resourceName);

            ConfigurationUnit unit;
            unit.Type(resourceNameWide);

            ValueSet directives;
            directives.Insert(s_Directive_Module, PropertyValue::CreateString(moduleNameWide));

            Utility::LocIndString description;
            if (dependentUnit.has_value())
            {
                description = Resource::String::ConfigureExportUnitDescription(Utility::LocIndView{ Utility::ConvertToUTF8(dependentUnit.value().Identifier()) });
            }
            else
            {
                description = Resource::String::ConfigureExportUnitDescription(Utility::LocIndView{ resourceName });
            }

            directives.Insert(s_Directive_Description, PropertyValue::CreateString(winrt::to_hstring(description.get())));
            unit.Metadata(directives);

            // Call processor to get settings for the unit.
            auto getResult = GetUnitSettings(context, unit);
            winrt::hresult resultCode = getResult.ResultInformation().ResultCode();
            if (FAILED(resultCode))
            {
                // Retry if it fails with not found in the case the module is a pre-released one.
                bool isPreRelease = false;
                if (resultCode == WINGET_CONFIG_ERROR_UNIT_NOT_FOUND_REPOSITORY)
                {
                    directives.Insert(s_Directive_AllowPrerelease, PropertyValue::CreateBoolean(true));
                    unit.Metadata(directives);

                    auto preReleaseResult = GetUnitSettings(context, unit);
                    if (SUCCEEDED(preReleaseResult.ResultInformation().ResultCode()))
                    {
                        isPreRelease = true;
                        getResult = preReleaseResult;
                    }
                    else
                    {
                        AICLI_LOG(Config, Error, << "Failed Get allowing prerelease modules");
                        LogFailedGetConfigurationUnitDetails(unit, preReleaseResult.ResultInformation());
                    }
                }

                if (!isPreRelease)
                {
                    OutputUnitRunFailure(context, unit, getResult.ResultInformation());
                    THROW_HR(WINGET_CONFIG_ERROR_GET_FAILED);
                }
            }

            unit.Settings(getResult.Settings());

            // GetUnitSettings will set it to Inform.
            unit.Intent(ConfigurationUnitIntent::Apply);

            // Add dependency if needed.
            if (dependentUnit.has_value())
            {
                auto dependencies = winrt::single_threaded_vector<winrt::hstring>();
                dependencies.Append(dependentUnit.value().Identifier());
                unit.Dependencies(std::move(dependencies));
            }

            return unit;
        }

        bool HistorySetMatchesInput(const ConfigurationSet& set, const std::string& foldedInput)
        {
            if (foldedInput.empty())
            {
                return false;
            }

            if (Utility::FoldCase(Utility::NormalizedString{ set.Name() }) == foldedInput)
            {
                return true;
            }

            std::ostringstream identifierStream;
            identifierStream << set.InstanceIdentifier();
            std::string identifier = identifierStream.str();
            THROW_HR_IF(E_UNEXPECTED, identifier.empty());

            std::size_t startPosition = 0;
            if (identifier[0] == '{' && foldedInput[0] != '{')
            {
                startPosition = 1;
            }

            std::string_view identifierView = identifier;
            identifierView = identifierView.substr(startPosition);

            return Utility::CaseInsensitiveStartsWith(identifierView, foldedInput);
        }

        Resource::LocString ToLocString(ConfigurationSetState state)
        {
            switch (state)
            {
            case ConfigurationSetState::Pending:
                return Resource::String::ConfigurationSetStatePending;
            case ConfigurationSetState::InProgress:
                return Resource::String::ConfigurationSetStateInProgress;
            case ConfigurationSetState::Completed:
                return Resource::String::ConfigurationSetStateCompleted;
            case ConfigurationSetState::Unknown:
            default:
                return Resource::String::ConfigurationSetStateUnknown;
            }
        }

        Resource::LocString ToLocString(ConfigurationUnitState state)
        {
            switch (state)
            {
            case ConfigurationUnitState::Pending:
                return Resource::String::ConfigurationUnitStatePending;
            case ConfigurationUnitState::InProgress:
                return Resource::String::ConfigurationUnitStateInProgress;
            case ConfigurationUnitState::Completed:
                return Resource::String::ConfigurationUnitStateCompleted;
            case ConfigurationUnitState::Skipped:
                return Resource::String::ConfigurationUnitStateSkipped;
            case ConfigurationUnitState::Unknown:
            default:
                return Resource::String::ConfigurationUnitStateUnknown;
            }
        }

        std::string_view ToString(ConfigurationChangeEventType type)
        {
            switch (type)
            {
            case ConfigurationChangeEventType::SetAdded:
                return "SetAdded";
            case ConfigurationChangeEventType::SetStateChanged:
                return "SetStateChanged";
            case ConfigurationChangeEventType::SetRemoved:
                return "SetRemoved";
            case ConfigurationChangeEventType::Unknown:
            default:
                return "Unknown";
            }
        }

        std::string_view ToString(ConfigurationUnitResultSource source)
        {
            switch (source)
            {
            case ConfigurationUnitResultSource::Internal:
                return "Internal";
            case ConfigurationUnitResultSource::ConfigurationSet:
                return "ConfigurationSet";
            case ConfigurationUnitResultSource::UnitProcessing:
                return "UnitProcessing";
            case ConfigurationUnitResultSource::SystemState:
                return "SystemState";
            case ConfigurationUnitResultSource::Precondition:
                return "Precondition";
            case ConfigurationUnitResultSource::None:
            default:
                return "None";
            }
        }
    }

    void CreateConfigurationProcessor(Context& context)
    {
        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        progressScope->Callback().SetProgressMessage(Resource::String::ConfigurationInitializing());

        anon::ConfigureProcessorForUse(context, ConfigurationProcessor{ anon::CreateConfigurationSetProcessorFactory(context) });
    }

    void CreateConfigurationProcessorWithoutFactory(Execution::Context& context)
    {
        anon::ConfigureProcessorForUse(context, ConfigurationProcessor{ IConfigurationSetProcessorFactory{ nullptr } });
    }

    void OpenConfigurationSet(Context& context)
    {
        if (context.Args.Contains(Args::Type::ConfigurationFile))
        {
            std::string argPath{ context.Args.GetArg(Args::Type::ConfigurationFile) };
            anon::OpenConfigurationSet(context, argPath, true);
        }
        else
        {
            THROW_HR_IF(E_UNEXPECTED, !context.Args.Contains(Args::Type::ConfigurationHistoryItem));

            context <<
                GetConfigurationSetHistory <<
                SelectSetFromHistory;
        }
    }

    void CreateOrOpenConfigurationSet(Context& context)
    {
        std::string argPath{ context.Args.GetArg(Args::Type::OutputFile) };

        if (std::filesystem::exists(argPath))
        {
            anon::OpenConfigurationSet(context, argPath, false);
        }
        else
        {
            // TODO: support other schema versions or pick up latest.
            ConfigurationSet set;
            set.SchemaVersion(L"0.2");

            std::wstring argPathWide = Utility::ConvertToUTF16(argPath);
            auto absolutePath = std::filesystem::weakly_canonical(std::filesystem::path{ argPathWide });
            anon::SetNameAndOrigin(set, absolutePath);

            context.Get<Data::ConfigurationContext>().Set(set);
        }
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
        auto unification = anon::CreateProgressCancellationUnification(std::move(progressScope), getDetailsOperation);

        bool suppressDetailsOutput = context.Args.Contains(Args::Type::ConfigurationAcceptWarning) && context.Args.Contains(Args::Type::ConfigurationSuppressPrologue);
        anon::OutputHelper outputHelper{ context };
        uint32_t unitsShown = 0;

        if (!suppressDetailsOutput)
        {
            getDetailsOperation.Progress([&](const IAsyncOperationWithProgress<GetConfigurationSetDetailsResult, GetConfigurationUnitDetailsResult>& operation, const GetConfigurationUnitDetailsResult&)
                {
                    auto threadContext = context.SetForCurrentThread();

                    unification.Reset();

                    auto unitResults = operation.GetResults().UnitResults();
                    for (unitsShown; unitsShown < unitResults.Size(); ++unitsShown)
                    {
                        GetConfigurationUnitDetailsResult unitResult = unitResults.GetAt(unitsShown);
                        anon::LogFailedGetConfigurationUnitDetails(unitResult.Unit(), unitResult.ResultInformation());
                        outputHelper.OutputConfigurationUnitInformation(unitResult.Unit());
                    }

                    progressScope = context.Reporter.BeginAsyncProgress(true);
                    progressScope->Callback().SetProgressMessage(gettingDetailString);
                    unification.Progress(std::move(progressScope));
                });
        }

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
        if (result && !suppressDetailsOutput)
        {
            auto unitResults = result.UnitResults();
            if (unitResults)
            {
                for (unitsShown; unitsShown < unitResults.Size(); ++unitsShown)
                {
                    GetConfigurationUnitDetailsResult unitResult = unitResults.GetAt(unitsShown);
                    anon::LogFailedGetConfigurationUnitDetails(unitResult.Unit(), unitResult.ResultInformation());
                    outputHelper.OutputConfigurationUnitInformation(unitResult.Unit());
                }
            }
        }

        // Handle any units that are NOT in the results (due to an exception part of the way through)
        if (!suppressDetailsOutput)
        {
            auto allUnits = configContext.Set().Units();
            for (unitsShown; unitsShown < allUnits.Size(); ++unitsShown)
            {
                ConfigurationUnit unit = allUnits.GetAt(unitsShown);
                outputHelper.OutputConfigurationUnitInformation(unit);
            }
        }

        if (outputHelper.ValuesTruncated)
        {
            // Using error to make this stand out from other warnings
            context.Reporter.Error() << Resource::String::ConfigurationWarningSetViewTruncated << std::endl;
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
            if (!context.Reporter.PromptForBoolResponse(promptString, Reporter::Level::Warning, false))
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
            anon::ApplyConfigurationSetProgressOutput progress{ context, applyOperation };

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
            anon::TestConfigurationSetProgressOutput progress{ context, testOperation };

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

                    anon::OutputConfigurationUnitHeader(context, unit, unit.Type());

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
        auto unification = anon::CreateProgressCancellationUnification(std::move(progressScope), getLocalDetailsOperation);

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
        unification = anon::CreateProgressCancellationUnification(std::move(progressScope), getCatalogDetailsOperation);

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
                    anon::OutputConfigurationUnitHeader(context, unit, unit.Type());

                    needsHeader = false;
                    foundIssue = true;
                }
            };

            if (anon::GetValueSetString(unit.Metadata(), anon::s_Directive_Module).empty())
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
                anon::OutputUnitRunFailure(context, unit, catalogUnitResult.ResultInformation());
                continue;
            }

            // If not already prerelease, try with prerelease and warn if found
            std::optional<bool> allowPrereleaseDirective = anon::GetValueSetBool(unit.Metadata(), anon::s_Directive_AllowPrerelease);
            if (!allowPrereleaseDirective || !allowPrereleaseDirective.value())
            {
                // Check if the configuration unit is prerelease but the author forgot it
                ConfigurationUnit clone = unit.Copy();
                clone.Metadata().Insert(anon::s_Directive_AllowPrerelease, PropertyValue::CreateBoolean(true));

                progressScope = context.Reporter.BeginAsyncProgress(true);
                progressScope->Callback().SetProgressMessage(gettingDetailString);

                auto getUnitDetailsOperation = configContext.Processor().GetUnitDetailsAsync(clone, ConfigurationUnitDetailFlags::Catalog);
                auto unitUnification = anon::CreateProgressCancellationUnification(std::move(progressScope), getUnitDetailsOperation);

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

    void ValidateConfigurationSetUnitContents(Execution::Context& context)
    {
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();
        auto units = configContext.Set().Units();
        auto validationOrder = anon::GetConfigurationSetUnitValidationOrder(units.GetView());

        Configuration::WingetDscModuleUnitValidator wingetUnitValidator;

        bool foundIssues = false;
        for (const auto index : validationOrder)
        {
            const ConfigurationUnit& unit = units.GetAt(index);
            auto moduleName = Utility::ConvertToUTF8(unit.Details().ModuleName());
            if (Utility::CaseInsensitiveEquals(wingetUnitValidator.ModuleName(), moduleName))
            {
                bool result = wingetUnitValidator.ValidateConfigurationSetUnit(context, unit);
                if (!result)
                {
                    foundIssues = true;
                }
            }
        }

        if (foundIssues)
        {
            // Indicate that it was not a total success
            AICLI_TERMINATE_CONTEXT(S_FALSE);
        }
    }

    void ValidateAllGoodMessage(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::ConfigurationValidationFoundNoIssues << std::endl;
    }

    void SearchSourceForPackageExport(Execution::Context& context)
    {
        if (!context.Args.Contains(Args::Type::ConfigurationExportAll) && !context.Args.Contains(Args::Type::ConfigurationExportPackageId))
        {
            // No package export needed.
            return;
        }

        context <<
            OpenSource() <<
            OpenCompositeSource(Repository::PredefinedSource::Installed);

        if (context.Args.Contains(Args::Type::ConfigurationExportAll))
        {
            context <<
                SearchSourceForMany <<
                HandleSearchResultFailures <<
                EnsureMatchesFromSearchResult(OperationType::Export) <<
                SelectVersionsToExport;
        }
        else if (context.Args.Contains(Args::Type::ConfigurationExportPackageId))
        {
            context.Args.AddArg(Args::Type::Id, context.Args.GetArg(Args::Type::ConfigurationExportPackageId));
            context <<
                SearchSourceForSingle <<
                Workflow::HandleSearchResultFailures <<
                Workflow::EnsureOneMatchFromSearchResult(OperationType::Export) <<
                SelectVersionsToExport;
        }
    }

    void PopulateConfigurationSetForExport(Execution::Context& context)
    {
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();

        // When exporting single WinGetPackage unit, the WinGetPackage unit can be used as a dependent unit for following configuration unit.
        // This is not used in export all scenario.
        std::optional<ConfigurationUnit> singlePackageUnit;

        for (const auto& source : context.Get<Execution::Data::PackageCollection>().Sources)
        {
            // Create WinGetSource unit for non well known source.
            std::optional<ConfigurationUnit> sourceUnit;
            if (!CheckForWellKnownSource(source.Details))
            {
                sourceUnit = anon::CreateWinGetSourceUnit(source);
                configContext.Set().Units().Append(sourceUnit.value());
            }

            for (const auto& package : source.Packages)
            {
                auto packageUnit = anon::CreateWinGetPackageUnit(package, source, context.Args.Contains(Args::Type::IncludeVersions), sourceUnit);
                configContext.Set().Units().Append(packageUnit);
                if (!singlePackageUnit)
                {
                    singlePackageUnit = packageUnit;
                }
            }
        }

        if (context.Args.Contains(Execution::Args::Type::ConfigurationExportModule, Execution::Args::Type::ConfigurationExportResource))
        {
            auto configUnit = anon::CreateConfigurationUnit(
                context,
                context.Args.GetArg(Args::Type::ConfigurationExportModule),
                context.Args.GetArg(Args::Type::ConfigurationExportResource),
                singlePackageUnit);

            configContext.Set().Units().Append(configUnit);
        }
    }

    void WriteConfigFile(Execution::Context& context)
    {
        try
        {
            std::string argPath{ context.Args.GetArg(Args::Type::OutputFile) };

            context.Reporter.Info() << Resource::String::ConfigurationExportAddingToFile(Utility::LocIndView{ argPath }) << std::endl;

            auto tempFilePath = Runtime::GetNewTempFilePath();

            {
                std::ofstream tempStream{ tempFilePath };
                tempStream << "# Created using winget configure export " << Runtime::GetClientVersion().get() << std::endl;
            }

            auto openAction = Streams::FileRandomAccessStream::OpenAsync(
                tempFilePath.wstring(),
                FileAccessMode::ReadWrite);

            auto stream = openAction.get();
            stream.Seek(stream.Size());

            ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();
            configContext.Set().Serialize(openAction.get());

            auto absolutePath = std::filesystem::weakly_canonical(std::filesystem::path{ argPath });
            std::filesystem::rename(tempFilePath, absolutePath);

            context.Reporter.Info() << Resource::String::ConfigurationExportSuccessful << std::endl;
        }
        catch (...)
        {
            context.Reporter.Error() << Resource::String::ConfigurationExportFailed << std::endl;
            throw;
        }
    }

    void GetConfigurationSetHistory(Execution::Context& context)
    {
        auto progressScope = context.Reporter.BeginAsyncProgress(true);

        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();
        configContext.History(configContext.Processor().GetConfigurationHistory());
    }

    void ShowConfigurationSetHistory(Execution::Context& context)
    {
        const auto& history = context.Get<Data::ConfigurationContext>().History();

        if (history.empty())
        {
            context.Reporter.Info() << Resource::String::ConfigurationHistoryEmpty << std::endl;
        }
        else
        {
            TableOutput<4> historyTable{ context.Reporter, { Resource::String::ConfigureListIdentifier, Resource::String::ConfigureListName, Resource::String::ConfigureListState, Resource::String::ConfigureListOrigin } };

            for (const auto& set : history)
            {
                winrt::hstring origin = set.Path();
                if (origin.empty())
                {
                    origin = set.Origin();
                }

                historyTable.OutputLine({ Utility::ConvertGuidToString(set.InstanceIdentifier()), Utility::ConvertToUTF8(set.Name()), anon::ToLocString(set.State()), Utility::ConvertToUTF8(origin)});
            }

            historyTable.Complete();
        }
    }

    void SelectSetFromHistory(Execution::Context& context)
    {
        ConfigurationContext& configContext = context.Get<Data::ConfigurationContext>();
        ConfigurationSet selectedSet{ nullptr };

        std::string foldedInput = Utility::FoldCase(context.Args.GetArg(Execution::Args::Type::ConfigurationHistoryItem));

        for (const ConfigurationSet& historySet : configContext.History())
        {
            if (anon::HistorySetMatchesInput(historySet, foldedInput))
            {
                if (selectedSet)
                {
                    selectedSet = nullptr;
                    break;
                }
                else
                {
                    selectedSet = historySet;
                }
            }
        }

        if (!selectedSet)
        {
            context.Reporter.Warn() << Resource::String::ConfigurationHistoryItemNotFound << std::endl;
            context << ShowConfigurationSetHistory;
            AICLI_TERMINATE_CONTEXT(WINGET_CONFIG_ERROR_HISTORY_ITEM_NOT_FOUND);
        }

        configContext.Set(std::move(selectedSet));
    }

    void RemoveConfigurationSetHistory(Execution::Context& context)
    {
        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        context.Get<Data::ConfigurationContext>().Set().Remove();
    }

    void SerializeConfigurationSetHistory(Execution::Context& context)
    {
        auto progressScope = context.Reporter.BeginAsyncProgress(true);
        std::filesystem::path absolutePath = std::filesystem::weakly_canonical(std::filesystem::path{ Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::OutputFile)) });
        auto openAction = Streams::FileRandomAccessStream::OpenAsync(absolutePath.wstring(), FileAccessMode::ReadWrite, StorageOpenOptions::None, Streams::FileOpenDisposition::CreateAlways);
        auto cancellationScope = progressScope->Callback().SetCancellationFunction([&]() { openAction.Cancel(); });
        auto outputStream = openAction.get();

        context.Get<Data::ConfigurationContext>().Set().Serialize(outputStream);
    }

    void ShowSingleConfigurationSetHistory(Execution::Context& context)
    {
        const auto& set = context.Get<Data::ConfigurationContext>().Set();

        // Output a table with name/value pairs for some of the set's properties. Example:
        // 
        // Field         Value
        // ----------------------------------------------------
        // Identifier    {7D5CF50E-F3C6-4333-BFE6-5A806F9EBA4E}
        // Name          Test Name
        // Origin        Test Origin
        // Path          Test Path
        // State         Completed
        // First Applied 2024-07-16 21:15:13.000
        // Apply Begun   2024-07-16 21:15:13.000
        // Apply Ended   2024-07-16 21:15:13.000
        Execution::TableOutput<2> table(context.Reporter, { Resource::String::SourceListField, Resource::String::SourceListValue });

        table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListIdentifier }, Utility::ConvertGuidToString(set.InstanceIdentifier()) });
        table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListName }, Utility::ConvertToUTF8(set.Name()) });
        table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListOrigin }, Utility::ConvertToUTF8(set.Origin()) });
        table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListPath }, Utility::ConvertToUTF8(set.Path()) });
        table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListState }, anon::ToLocString(set.State()) });
        table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListFirstApplied }, Utility::TimePointToString(winrt::clock::to_sys(set.FirstApply())) });

        auto applyBegun = set.ApplyBegun();
        if (applyBegun != winrt::clock::time_point{})
        {
            table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListApplyBegun }, Utility::TimePointToString(winrt::clock::to_sys(applyBegun)) });
        }

        auto applyEnded = set.ApplyEnded();
        if (applyEnded != winrt::clock::time_point{})
        {
            table.OutputLine({ Resource::LocString{ Resource::String::ConfigureListApplyEnded }, Utility::TimePointToString(winrt::clock::to_sys(applyEnded)) });
        }

        table.Complete();

        context.Reporter.Info() << std::endl;

        // Output a table with unit state information. Groups are represented by indentation beneath their parent unit. Example:
        //
        // Unit                        State     Result     Details
        // ------------------------------------------------------------
        // Module/Resource [Name]      Completed 0x00000000
        // Module2/Resource [Group]    Completed 0x00000000
        // |-Module3/Resource [Child1] Completed 0x00000000
        // |---Module4/Resource2       Completed 0x80004005 I failed :(
        // |-Module3/Resource [Child2] Completed 0x00000000
        Execution::TableOutput<4> unitTable(context.Reporter, { Resource::String::ConfigureListUnit, Resource::String::ConfigureListState, Resource::String::ConfigureListResult, Resource::String::ConfigureListResultDescription });

        struct UnitSiblings
        {
            size_t Depth = 0;
            size_t Current = 0;
            std::vector<ConfigurationUnit> Siblings;
        };

        std::vector<UnitSiblings> stack;

        {
            UnitSiblings initial;
            auto units = set.Units();
            initial.Siblings.resize(units.Size());
            units.GetMany(0, initial.Siblings);
            stack.emplace_back(std::move(initial));
        }

        // Each item on the stack is a list of sibling units.
        // Each iteration, we process the Current sibling from the group on top of the stack.
        // If it is a group, we add its children as a new stack item to be processed next.
        while (!stack.empty())
        {
            UnitSiblings& currentSiblings = stack.back();

            if (currentSiblings.Current >= currentSiblings.Siblings.size())
            {
                stack.pop_back();
                continue;
            }

            ConfigurationUnit& currentUnit = currentSiblings.Siblings[currentSiblings.Current++];

            std::ostringstream unitStream;

            if (currentSiblings.Depth)
            {
                unitStream << '|' << std::string((currentSiblings.Depth * 2) - 1, '-');
            }

            unitStream << Utility::ConvertToUTF8(currentUnit.Type());

            auto identifier = currentUnit.Identifier();
            if (!identifier.empty())
            {
                unitStream << " [" << Utility::ConvertControlCodesToPictures(Utility::ConvertToUTF8(identifier)) << ']';
            }

            auto resultInformation = currentUnit.ResultInformation();
            std::ostringstream resultStream;
            std::string resultDetails;

            if (resultInformation)
            {
                resultStream << "0x" << Logging::SetHRFormat << resultInformation.ResultCode();

                auto description = resultInformation.Description();
                if (description.empty())
                {
                    description = resultInformation.Details();
                }

                resultDetails = Utility::ConvertControlCodesToPictures(Utility::ConvertToUTF8(description));
            }

            unitTable.OutputLine({ std::move(unitStream).str(), anon::ToLocString(currentUnit.State()), std::move(resultStream).str(), std::move(resultDetails) });

            if (currentUnit.IsGroup())
            {
                UnitSiblings unitChildren;
                unitChildren.Depth = currentSiblings.Depth + 1;
                auto units = currentUnit.Units();
                unitChildren.Siblings.resize(units.Size());
                units.GetMany(0, unitChildren.Siblings);
                stack.emplace_back(std::move(unitChildren));
            }
        }

        unitTable.Complete();
    }

    void CompleteConfigurationHistoryItem(Execution::Context& context)
    {
        const std::string& word = context.Get<Data::CompletionData>().Word();
        auto stream = context.Reporter.Completion();

        for (const auto& historyItem : ConfigurationProcessor{ IConfigurationSetProcessorFactory{ nullptr } }.GetConfigurationHistory())
        {
            std::ostringstream identifierStream;
            identifierStream << historyItem.InstanceIdentifier();
            std::string identifier = identifierStream.str();

            if (word.empty() || Utility::CaseInsensitiveContainsSubstring(identifier, word))
            {
                stream << '"' << identifier << '"' << std::endl;
            }

            std::string name = Utility::ConvertToUTF8(historyItem.Name());

            if (word.empty() || Utility::CaseInsensitiveStartsWith(name, word))
            {
                stream << '"' << name << '"' << std::endl;
            }
        }
    }

    void MonitorConfigurationStatus(Execution::Context& context)
    {
        auto& configurationContext = context.Get<Data::ConfigurationContext>();

        std::mutex activeSetMutex;
        ConfigurationSet activeSet{ nullptr };
        decltype(activeSet.ConfigurationSetChange(winrt::auto_revoke, nullptr)) activeSetRevoker;

        auto setChangeHandler = [&](const ConfigurationSet& set, const ConfigurationSetChangeData& changeData)
            {
                if (changeData.Change() == ConfigurationSetChangeEventType::SetStateChanged)
                {
                    context.Reporter.Info() << "(SetStateChanged) " << set.InstanceIdentifier() << " :: " << anon::ToLocString(changeData.SetState()) << std::endl;
                }
                else if (changeData.Change() == ConfigurationSetChangeEventType::UnitStateChanged)
                {
                    context.Reporter.Info() << "(UnitStateChanged) " << changeData.Unit().InstanceIdentifier() << " :: " << anon::ToLocString(changeData.UnitState()) << std::endl;

                    auto resultInformation = changeData.ResultInformation();
                    if (resultInformation)
                    {
                        context.Reporter.Info() << "    [" << anon::ToString(resultInformation.ResultSource()) << "] :: 0x" << Logging::SetHRFormat << resultInformation.ResultCode() << std::endl;
                    }
                }
            };

        auto setActiveSet = [&](const ConfigurationSet& set, bool force)
            {
                std::lock_guard<std::mutex> lock{ activeSetMutex };

                if (force || !activeSet)
                {
                    activeSet = set;
                    activeSetRevoker = activeSet.ConfigurationSetChange(winrt::auto_revoke, setChangeHandler);
                }
            };

        auto processorRevoker = configurationContext.Processor().ConfigurationChange(winrt::auto_revoke, [&](const ConfigurationSet& set, const ConfigurationChangeData& changeData)
            {
                context.Reporter.Info() << '[' << anon::ToString(changeData.Change()) << "] " << changeData.InstanceIdentifier() << " :: " << anon::ToLocString(changeData.State()) << std::endl;

                if (changeData.Change() == ConfigurationChangeEventType::SetStateChanged && changeData.State() == ConfigurationSetState::InProgress)
                {
                    setActiveSet(set, true);
                }
            });

        for (ConfigurationSet& historySet : configurationContext.History())
        {
            if (historySet.State() == ConfigurationSetState::InProgress)
            {
                setActiveSet(historySet, false);
            }
        }

        for (;;)
        {
            std::this_thread::sleep_for(250ms);
            if (context.IsTerminated())
            {
                return;
            }
        }
    }
}
