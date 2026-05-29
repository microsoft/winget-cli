// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ErrorCommand.h"
#include "AppInstallerStrings.h"
#include "AppInstallerErrors.h"
#include "VTSupport.h"

#include <filesystem>

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

        struct ErrorGroup
        {
            std::string_view Name;
            WORD MinCode; // HRESULT_CODE, inclusive
            WORD MaxCode; // HRESULT_CODE, inclusive
        };

        // Groups matching the sections in the published returnCodes.md documentation.
        constexpr ErrorGroup s_errorGroups[] =
        {
            { "General Errors",                       0x0001, 0x00FF },
            { "Install errors.",                      0x0100, 0x01FF },
            { "Check for package installed status",   0x0200, 0x02FF },
            { "Configuration Errors",                 0xC000, 0xC0FF },
            { "Configuration Processor Errors",       0xC100, 0xC1FF },
        };

        constexpr std::string_view s_uncategorizedGroupName = "Uncategorized";

        // Writes the markdown table for a group of errors to the given stream.
        void WriteMarkdownGroup(std::ostream& out, std::string_view groupName, std::vector<const Errors::HResultInformation*>& errors)
        {
            if (errors.empty())
            {
                return;
            }

            std::sort(errors.begin(), errors.end(),
                [](const Errors::HResultInformation* a, const Errors::HResultInformation* b) { return HRESULT_CODE(a->Value()) < HRESULT_CODE(b->Value()); });

            out << "\n## " << groupName << "\n\n";
            out << "| Hex | Decimal | Symbol | Description |\n";
            out << "|-------------|-------------|-------------|-------------|\n";

            for (const auto* error : errors)
            {
                HRESULT hr = error->Value();
                char hexBuf[11];
                sprintf_s(hexBuf, "0x%08X", static_cast<uint32_t>(hr));

                out << "| " << hexBuf
                    << " | " << static_cast<int32_t>(hr)
                    << " | " << error->Symbol()
                    << " | " << error->GetDescription()
                    << " |\n";
            }
        }

        void OutputErrorMarkdown(Execution::Context& context)
        {
            auto allErrors = Errors::GetWinGetErrors();

            // Categorize each error into its group; the last slot holds uncategorized errors.
            static constexpr size_t s_errorGroupsCount = ARRAYSIZE(s_errorGroups);
            std::vector<std::vector<const Errors::HResultInformation*>> groupedErrors(s_errorGroupsCount + 1);

            for (const auto& error : allErrors)
            {
                HRESULT hr = error->Value();
                WORD code = static_cast<WORD>(HRESULT_CODE(hr));

                bool placed = false;
                for (size_t i = 0; i < s_errorGroupsCount; ++i)
                {
                    if (code >= s_errorGroups[i].MinCode && code <= s_errorGroups[i].MaxCode)
                    {
                        groupedErrors[i].push_back(error.get());
                        placed = true;
                        break;
                    }
                }

                if (!placed)
                {
                    groupedErrors[s_errorGroupsCount].push_back(error.get());
                }
            }

            std::filesystem::path outputPath{ Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::OutputFile)) };
            std::ofstream out{ outputPath };
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_OPEN_FAILED), !out);

            std::time_t now = std::time(nullptr);
            std::tm tm{};
            localtime_s(&tm, &now);
            char dateBuf[11];
            strftime(dateBuf, sizeof(dateBuf), "%m/%d/%Y", &tm);

            out <<
                "---\n"
                "title: WinGet HRESULT Codes\n"
                "description: WinGet return codes and their meanings\n"
                "ms.date: " << dateBuf << "\n"
                "ms.topic: article\n"
                "ms.localizationpriority: low\n"
                "---\n"
                "\n"
                "> [!NOTE]\n"
                "> This file is generated by running: winget error --output <file>\n"
                "\n"
                "# Return Codes\n";

            for (size_t i = 0; i < ARRAYSIZE(s_errorGroups); ++i)
            {
                WriteMarkdownGroup(out, s_errorGroups[i].Name, groupedErrors[i]);
            }

            WriteMarkdownGroup(out, s_uncategorizedGroupName, groupedErrors[s_errorGroupsCount]);
        }
    }

    std::vector<Argument> ErrorCommand::GetArguments() const
    {
        return {
            Argument{ Execution::Args::Type::ErrorInput, Resource::String::ErrorInputArgumentDescription, ArgumentType::Positional },
            Argument{ Execution::Args::Type::OutputFile, Resource::String::ErrorOutputFileArgumentDescription, ArgumentType::Standard },
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

    void ErrorCommand::ValidateArgumentsInternal(Execution::Args& execArgs) const
    {
        if (execArgs.Contains(Execution::Args::Type::OutputFile) && execArgs.Contains(Execution::Args::Type::ErrorInput))
        {
            throw CommandException(Resource::String::ErrorOutputFileConflictsWithInput);
        }

        if (!execArgs.Contains(Execution::Args::Type::OutputFile) && !execArgs.Contains(Execution::Args::Type::ErrorInput))
        {
            throw CommandException(Resource::String::ErrorRequiresInputOrOutputFile);
        }
    }

    void ErrorCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::OutputFile))
        {
            OutputErrorMarkdown(context);
            return;
        }

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
