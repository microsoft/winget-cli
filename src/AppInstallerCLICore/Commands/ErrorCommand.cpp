// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ErrorCommand.h"
#include "AppInstallerStrings.h"
#include "AppInstallerErrors.h"
#include "VTSupport.h"

namespace AppInstaller::CLI
{
    namespace
    {
        // 0x12345678 : SYMBOL_VALUE
        // Descriptive text
        void OutputHResultInformation(Execution::Context& context, const Errors::HResultInformation& error)
        {
            auto info = context.Reporter.Info();
            info << VirtualTerminal::TextFormat::Foreground::Bright << "0x" << VirtualTerminal::TextFormat::Foreground::Bright << Logging::SetHRFormat << error.Value();
            if (!error.Symbol().empty())
            {
                info << " : "_liv << VirtualTerminal::TextFormat::Foreground::BrightCyan << error.Symbol();
            }
            info << std::endl;
            info << error.GetDescription() << std::endl;
        }
    }

    std::vector<Argument> ErrorCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ErrorInput, Resource::String::ErrorInputArgumentDescription, ArgumentType::Positional, true },
        };
    }

    Resource::LocString ErrorCommand::ShortDescription() const
    {
        return { Resource::String::ErrorCommandShortDescription };
    }

    Resource::LocString ErrorCommand::LongDescription() const
    {
        return { Resource::String::ErrorCommandLongDescription };
    }

    void ErrorCommand::ExecuteInternal(Execution::Context& context) const
    {
        std::string input{ context.Args.GetArg(Execution::Args::Type::ErrorInput) };

        const char* begin = input.c_str();
        char* end = nullptr;
        errno = 0;
        HRESULT errorNumber = strtol(begin, &end, 0);

        if (errno == ERANGE)
        {
            errno = 0;
            unsigned long unsignedError = strtoul(begin, &end, 0);

            if (errno == ERANGE)
            {
                context.Reporter.Error() << Resource::String::ErrorNumberIsTooLarge << std::endl;
                AICLI_TERMINATE_CONTEXT(E_INVALIDARG);
            }

            errorNumber = static_cast<HRESULT>(unsignedError);
        }

        // If the entire string was consumed as a number, treat it as an HRESULT
        if (static_cast<size_t>(end - begin) == input.length())
        {
            auto error = Errors::HResultInformation::Find(errorNumber);
            if (error)
            {
                OutputHResultInformation(context, *error);
            }
        }
        // otherwise, treat it as a string and search our error list
        else
        {
            auto errors = Errors::HResultInformation::Find(Utility::Trim(input));
            for (const auto& error : errors)
            {
                OutputHResultInformation(context, *error);
            }
        }
    }
}
