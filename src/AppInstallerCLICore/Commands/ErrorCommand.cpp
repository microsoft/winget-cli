// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ErrorCommand.h"
#include "AppInstallerStrings.h"

namespace AppInstaller::CLI
{
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
            context.Reporter.Error() << Resource::String::ErrorNumberIsTooLarge << std::endl;
            AICLI_TERMINATE_CONTEXT(E_INVALIDARG);
        }

        // If the entire string was consumed as a number, treat it as an HRESULT
        if (static_cast<size_t>(end - begin) == input.length())
        {
            // TODO: Numeric search
        }
        // otherwise, treat it as a string and search our error list
        else
        {
            Utility::Trim(input);
            // TODO: String search
        }
    }
}
