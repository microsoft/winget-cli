// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscCommandBase.h"
#include "DscCommand.h"
#include <winget/Runtime.h>
#include <winget/StdErrLogger.h>

#define WINGET_DSC_FUNCTION_FOREACH(_macro_) \
    _macro_(Get); \
    _macro_(Set); \
    _macro_(WhatIf); \
    _macro_(Test); \
    _macro_(Delete); \
    _macro_(Export); \
    _macro_(Validate); \
    _macro_(Resolve); \
    _macro_(Adapter); \
    _macro_(Schema); \

namespace AppInstaller::CLI
{
    namespace
    {
        std::string GetFunctionManifestString(DscFunctions function)
        {
            THROW_HR_IF(E_INVALIDARG, !WI_IsSingleFlagSet(function));

            switch (function)
            {
            case DscFunctions::Get: return "get";
            case DscFunctions::Set: return "set";
            case DscFunctions::WhatIf: return "whatIf";
            case DscFunctions::Test: return "test";
            case DscFunctions::Delete: return "delete";
            case DscFunctions::Export: return "export";
            case DscFunctions::Validate: return "validate";
            case DscFunctions::Resolve: return "resolve";
            case DscFunctions::Adapter: return "adapter";
            case DscFunctions::Schema: return "schema";
            }

            THROW_HR(E_NOTIMPL);
        }

        std::string GetFunctionArgumentString(DscFunctions function)
        {
            return std::string{ "--" } + GetFunctionManifestString(function);
        }

        bool FunctionSpecifiesInput(DscFunctions function)
        {
            switch (function)
            {
            case DscFunctions::Get:
            case DscFunctions::Set:
            case DscFunctions::WhatIf:
            case DscFunctions::Test:
            case DscFunctions::Delete:
            case DscFunctions::Export:
            case DscFunctions::Validate:
            case DscFunctions::Resolve:
                return true;
            }

            return false;
        }

        bool FunctionIsSetLike(DscFunctions function)
        {
            switch (function)
            {
            case DscFunctions::Set:
            case DscFunctions::WhatIf:
                return true;
            }

            return false;
        }

        bool FunctionSpecifiesReturn(DscFunctions function)
        {
            switch (function)
            {
            case DscFunctions::Set:
            case DscFunctions::WhatIf:
            case DscFunctions::Test:
                return true;
            }

            return false;
        }

        std::optional<std::string> GetReturnType(DscFunctionModifiers modifiers)
        {
            if (WI_IsFlagSet(modifiers, DscFunctionModifiers::ReturnsStateAndDiff))
            {
                return "stateAndDiff";
            }

            if (WI_IsFlagSet(modifiers, DscFunctionModifiers::ReturnsState))
            {
                return "state";
            }

            return std::nullopt;
        }

        Json::Value CreateJsonDefinitionFor(std::string_view name, DscFunctions function, DscFunctionModifiers modifiers)
        {
            THROW_HR_IF(E_INVALIDARG, !WI_IsSingleFlagSet(function));
            THROW_HR_IF(E_NOTIMPL, function == DscFunctions::Adapter);

            Json::Value result{ Json::ValueType::objectValue };

#ifndef USE_PROD_CLSIDS
            result["executable"] = "wingetdev";
#else
            result["executable"] = "winget";
#endif

            Json::Value args{ Json::ValueType::arrayValue };
            args.append(std::string{ DscCommand::StaticName() });
            args.append(std::string{ name });
            args.append(GetFunctionArgumentString(function));
            result["args"] = std::move(args);

            if (FunctionSpecifiesInput(function))
            {
                result["input"] = "stdin";
            }

            if (FunctionIsSetLike(function))
            {
                if (WI_IsFlagSet(modifiers, DscFunctionModifiers::ImplementsPretest))
                {
                    result["implementsPretest"] = true;
                }

                if (WI_IsFlagSet(modifiers, DscFunctionModifiers::HandlesExist))
                {
                    result["handlesExist"] = true;
                }
            }

            if (FunctionSpecifiesReturn(function))
            {
                std::optional<std::string> returnType = GetReturnType(modifiers);

                if (returnType)
                {
                    result["return"] = returnType.value();
                }
            }

            if (function == DscFunctions::Schema)
            {
                Json::Value newResult{ Json::ValueType::objectValue };
                newResult["command"] = std::move(result);
                result = std::move(newResult);
            }

            return result;
        }
    }

    DscCommandBase::DscCommandBase(std::string_view parent, std::string_view resourceName, DscResourceKind kind, DscFunctions functions, DscFunctionModifiers modifiers) :
        Command(resourceName, parent, CommandOutputFlags::IgnoreSettingsWarnings), m_kind(kind), m_functions(functions), m_modifiers(modifiers)
    {
        // Limits on current implementation
        THROW_HR_IF(E_NOTIMPL, kind != DscResourceKind::Resource);
        THROW_HR_IF(E_NOTIMPL, WI_IsFlagSet(functions, DscFunctions::Adapter));
    }

    std::vector<Argument> DscCommandBase::GetArguments() const
    {
        std::vector<Argument> result;

#define WINGET_DSC_FUNCTION_ARGUMENT(_function_) \
        if (WI_IsFlagSet(m_functions, DscFunctions::_function_)) \
        { \
            result.emplace_back(Execution::Args::Type::DscResourceFunction ## _function_, Resource::String::DscResourceFunctionDescription ## _function_, ArgumentType::Flag); \
        }

        WINGET_DSC_FUNCTION_FOREACH(WINGET_DSC_FUNCTION_ARGUMENT);

#undef WINGET_DSC_FUNCTION_ARGUMENT

        result.emplace_back(Execution::Args::Type::DscResourceFunctionManifest, Resource::String::DscResourceFunctionDescriptionManifest, ArgumentType::Flag);
        result.emplace_back(Execution::Args::Type::OutputFile, Resource::String::OutputFileArgumentDescription, ArgumentType::Standard);

        return result;
    }

    Utility::LocIndView DscCommandBase::HelpLink() const
    {
        return "https://aka.ms/winget-dsc-resources"_liv;
    }

    void DscCommandBase::ExecuteInternal(Execution::Context& context) const
    {
        context.Reporter.SetChannel(Execution::Reporter::Channel::Json);
        Logging::StdErrLogger::Add();

#define WINGET_DSC_FUNCTION_ARGUMENT(_function_) \
        if (context.Args.Contains(Execution::Args::Type::DscResourceFunction ## _function_)) \
        { \
            return ResourceFunction ## _function_(context); \
        }

        WINGET_DSC_FUNCTION_FOREACH(WINGET_DSC_FUNCTION_ARGUMENT);
        WINGET_DSC_FUNCTION_ARGUMENT(Manifest);

#undef WINGET_DSC_FUNCTION_ARGUMENT
    }

#define WINGET_DSC_FUNCTION_METHOD(_function_) \
    void DscCommandBase::ResourceFunction ## _function_(Execution::Context&) const \
    { \
        THROW_HR(E_NOTIMPL); \
    } \

    WINGET_DSC_FUNCTION_FOREACH(WINGET_DSC_FUNCTION_METHOD);

    void DscCommandBase::WriteManifest(Execution::Context& context, const std::filesystem::path& filePath) const
    {
        Json::Value json{ Json::ValueType::objectValue };

        // TODO: Move to release schema when released (there should be an aka.ms link as well, but it wasn't active yet)
        //json["$schema"] = "https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/v3/bundled/resource/manifest.json";
        json["$schema"] = "https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2024/04/bundled/resource/manifest.json";
        json["type"] = std::string{ ModuleName() } + '/' + ResourceType();
        json["description"] = LongDescription().get();
        json["version"] = Runtime::GetClientVersion().get();

        Json::Value tags{ Json::ValueType::arrayValue };
        tags.append("WinGet");
        json["tags"] = std::move(tags);

#define WINGET_DSC_FUNCTION_MANIFEST(_function_) \
        if (WI_IsFlagSet(m_functions, DscFunctions::_function_)) \
        { \
            json[GetFunctionManifestString(DscFunctions::_function_)] = CreateJsonDefinitionFor(Name(), DscFunctions::_function_, m_modifiers); \
        }

        WINGET_DSC_FUNCTION_FOREACH(WINGET_DSC_FUNCTION_MANIFEST);

#undef WINGET_DSC_FUNCTION_MANIFEST

        Json::StreamWriterBuilder writerBuilder;
        writerBuilder.settings_["indentation"] = "  ";
        std::string jsonString = Json::writeString(writerBuilder, json);

        if (!filePath.empty())
        {
            std::ofstream stream{ filePath, std::ios::binary };
            stream.write(jsonString.c_str(), jsonString.length());
        }
        else
        {
            context.Reporter.Json() << jsonString;
        }
    }

    void DscCommandBase::ResourceFunctionManifest(Execution::Context& context) const
    {
        std::filesystem::path path;
        if (context.Args.Contains(Execution::Args::Type::OutputFile))
        {
            path = std::filesystem::path{ Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::OutputFile)) };
        }
        WriteManifest(context, path);
    }

#undef WINGET_DSC_FUNCTION_METHOD

    std::optional<Json::Value> DscCommandBase::GetJsonFromInput(Execution::Context& context, bool terminateContextOnError) const
    {
        // Don't attempt to read from an interactive stream as this will just block
        if (!context.Reporter.InputStreamIsInteractive())
        {
            AICLI_LOG(CLI, Verbose, << "Reading Json from input stream...");

            Json::Value result;
            Json::CharReaderBuilder builder;
            Json::String errors;
            if (Json::parseFromStream(builder, context.Reporter.RawInputStream(), &result, &errors))
            {
                AICLI_LOG(CLI, Info, << "Json from input stream:\n" << Json::writeString(Json::StreamWriterBuilder{}, result));
                return result;
            }

            AICLI_LOG(CLI, Error, << "Failed to read input JSON: " << errors);
        }

        if (terminateContextOnError)
        {
            AICLI_TERMINATE_CONTEXT_RETURN(APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE, std::nullopt);
        }
        else
        {
            return std::nullopt;
        }
    }

    void DscCommandBase::WriteJsonOutputLine(Execution::Context& context, const Json::Value& value) const
    {
        Json::StreamWriterBuilder writerBuilder;
        writerBuilder.settings_["indentation"] = "";
        writerBuilder.settings_["commentStyle"] = "None";
        writerBuilder.settings_["emitUTF8"] = true;
        context.Reporter.Json() << Json::writeString(writerBuilder, value) << std::endl;
    }
}
