// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationFlow.h"
#include <winrt/Microsoft.Management.Configuration.h>

using namespace AppInstaller::CLI::Execution;
using namespace winrt::Microsoft::Management::Configuration;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Storage;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
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

        std::string_view ToString(ConfigurationUnitIntent intent)
        {
            switch (intent)
            {
            case ConfigurationUnitIntent::Assert: return "Assert";
            case ConfigurationUnitIntent::Inform: return "Inform";
            case ConfigurationUnitIntent::Apply: return "Apply";
            default: return "Unknown";
            }
        }

        void OutputValueSet(OutputStream& out, const ValueSet& valueSet, size_t indent)
        {
            std::string indentString(indent, ' ');

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
                        out << "[PropertyType=" << property.Type() << "]\n";
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

    void GetConfigurationSetDetails(Context& context)
    {
        UNREFERENCED_PARAMETER(context);
    }

    void ShowConfigurationSet(Context& context)
    {
        // TODO: Given that this currently just dumps the data from the file, which can easily be read, lean more in to using
        //       the details if they can be acquired. Focus on information that would enable the user to make a decision
        //       about both the effect that the unit would have and the trustworthiness of the unit processor.
        OutputStream out = context.Reporter.Info();

        for (const ConfigurationUnit& unit : context.Get<Data::ConfigurationContext>().Set().ConfigurationUnits())
        {
            out << "Configuration Unit: " << Utility::ConvertToUTF8(unit.UnitName()) << '\n';

            if (!unit.Identifier().empty())
            {
                out << "  Identifier: " << Utility::ConvertToUTF8(unit.Identifier()) << '\n';
            }

            out << "  Intent: " << ToString(unit.Intent()) << '\n';

            auto dependencies = unit.Dependencies();
            if (dependencies.Size() > 0)
            {
                out << "  Dependencies:\n";
                for (const winrt::hstring& dependency : dependencies)
                {
                    out << "    " << Utility::ConvertToUTF8(dependency) << '\n';
                }
            }

            ValueSet directives = unit.Directives();
            if (directives.Size() > 0)
            {
                out << "  Directives:\n";
                OutputValueSet(out, directives, 4);
            }

            ValueSet settings = unit.Settings();
            if (settings.Size() > 0)
            {
                out << "  Settings:\n";
                OutputValueSet(out, settings, 4);
            }
        }
    }

    void ShowConfigurationSetConflicts(Execution::Context& context)
    {
        UNREFERENCED_PARAMETER(context);
    }

    void ConfirmConfigurationProcessing(Execution::Context& context)
    {
        UNREFERENCED_PARAMETER(context);
    }

    void ApplyConfigurationSet(Execution::Context& context)
    {
        UNREFERENCED_PARAMETER(context);
    }
}
