// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceCommand.h"
#include "Localization.h"


namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_SourceCommand_ArgName_Name = "name";
    constexpr std::string_view s_SourceCommand_ArgName_Type = "type";
    constexpr std::string_view s_SourceCommand_ArgName_Arg = "arg";

    std::vector<std::unique_ptr<Command>> SourceCommand::GetCommands() const
    {
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<SourceAddCommand>(),
            std::make_unique<SourceListCommand>(),
            });
    }

    std::string SourceCommand::ShortDescription() const
    {
        return LOCME("Manage sources of applications");
    }

    std::vector<std::string> SourceCommand::GetLongDescription() const
    {
        return {
            LOCME("Manage sources of applications"),
        };
    }

    void SourceCommand::ExecuteInternal(Invocation&, std::ostream& out, std::istream&) const
    {
        OutputHelp(out);
    }

    std::vector<Argument> SourceAddCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, LOCME("Name of the source for future reference"), ArgumentType::Positional, true },
            Argument{ s_SourceCommand_ArgName_Type, LOCME("Type of the source"), ArgumentType::Positional, true },
            Argument{ s_SourceCommand_ArgName_Arg, LOCME("Argument given to the source"), ArgumentType::Positional, true },
        };
    }

    std::string SourceAddCommand::ShortDescription() const
    {
        return LOCME("Add a new source");
    }

    std::vector<std::string> SourceAddCommand::GetLongDescription() const
    {
        return {
            LOCME("Add a new source"),
        };
    }

    void SourceAddCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream&) const
    {
        std::string name = *inv.GetArg(s_SourceCommand_ArgName_Name);
        std::string type = *inv.GetArg(s_SourceCommand_ArgName_Type);
        std::string arg = *inv.GetArg(s_SourceCommand_ArgName_Arg);

        out << LOCME("Adding source:") << std::endl;
        out << "  " << name << std::endl;
        out << "  " << type << std::endl;
        out << "  " << arg << std::endl;

        // TODO: Needs to be hooked up to a reporter when real source construction happens.
        Repository::AddSource(std::move(name), std::move(type), std::move(arg));

        out << LOCME("Done") << std::endl;
    }

    std::vector<Argument> SourceListCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, LOCME("Name of the source to list full details for"), ArgumentType::Positional, false },
        };
    }

    std::string SourceListCommand::ShortDescription() const
    {
        return LOCME("List current sources");
    }

    std::vector<std::string> SourceListCommand::GetLongDescription() const
    {
        return {
            LOCME("List current sources"),
        };
    }

    void SourceListCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream&) const
    {
        std::vector<Repository::SourceDetails> sources = Repository::GetSources();

        if (inv.Contains(s_SourceCommand_ArgName_Name))
        {
            const std::string& name = *inv.GetArg(s_SourceCommand_ArgName_Name);
            auto itr = std::find_if(sources.begin(), sources.end(), [name](const Repository::SourceDetails& sd) { return Utility::CaseInsensitiveEquals(sd.Name, name); });

            if (itr == sources.end())
            {
                out << LOCME("No source with the given name was found: ") << name << std::endl;
            }
            else
            {
                out << LOCME("Name") << ": " << itr->Name << std::endl;
                out << LOCME("Type") << ": " << itr->Type << std::endl;
                out << LOCME("Arg") << ": " << itr->Arg << std::endl;
                out << LOCME("Data") << ": " << itr->Data << std::endl;
                if (itr->LastUpdateTime == Utility::ConvertUnixEpochToSystemClock(0))
                {
                    out << LOCME("Last Update") << ": <never>" << std::endl;
                }
                else
                {
                    out << LOCME("Last Update") << ": " << itr->LastUpdateTime << std::endl;
                }
            }
        }
        else
        {
            out << LOCME("Current sources:") << std::endl;

            if (sources.empty())
            {
                out << LOCME("  <none>") << std::endl;
            }
            else
            {
                for (const auto& source : sources)
                {
                    out << "  " << source.Name << " => " << source.Arg << std::endl;
                }
            }
        }
    }
}
