// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceCommand.h"
#include "Localization.h"
#include "Workflows/WorkflowReporter.h"


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
            std::make_unique<SourceUpdateCommand>(),
            std::make_unique<SourceRemoveCommand>(),
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
            Argument{ s_SourceCommand_ArgName_Arg, LOCME("Argument given to the source"), ArgumentType::Positional, true },
            Argument{ s_SourceCommand_ArgName_Type, LOCME("Type of the source"), ArgumentType::Positional, false },
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

    void SourceAddCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const
    {
        std::string name = *inv.GetArg(s_SourceCommand_ArgName_Name);
        std::string arg = *inv.GetArg(s_SourceCommand_ArgName_Arg);
        std::string type;
        if (inv.Contains(s_SourceCommand_ArgName_Type))
        {
            type = *inv.GetArg(s_SourceCommand_ArgName_Type);
        }

        out << LOCME("Adding source:") << std::endl;
        out << "  " << LOCME("Name: ") << name << std::endl;
        out << "  " << LOCME("Arg: ") << arg << std::endl;
        if (!type.empty())
        {
            out << "  " << LOCME("Type: ") << type << std::endl;
        }

        Workflow::WorkflowReporter reporter(out, in);
        reporter.ExecuteWithProgress(std::bind(Repository::AddSource, std::move(name), std::move(type), std::move(arg), std::placeholders::_1));

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

    std::vector<Argument> SourceUpdateCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, LOCME("Name of the source to update"), ArgumentType::Positional, false },
        };
    }

    std::string SourceUpdateCommand::ShortDescription() const
    {
        return LOCME("Update current sources");
    }

    std::vector<std::string> SourceUpdateCommand::GetLongDescription() const
    {
        return {
            LOCME("Update current sources"),
        };
    }

    void SourceUpdateCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const
    {
        Workflow::WorkflowReporter reporter(out, in);

        if (inv.Contains(s_SourceCommand_ArgName_Name))
        {
            const std::string& name = *inv.GetArg(s_SourceCommand_ArgName_Name);
            out << LOCME("Updating source: ") << name << "..." << std::flush;
            if (!reporter.ExecuteWithProgress(std::bind(Repository::UpdateSource, name, std::placeholders::_1)))
            {
                out << std::endl << LOCME("  Could not find a source by that name.") << std::endl;
            }
            else
            {
                out << LOCME(" Done") << std::endl;
            }
        }
        else
        {
            out << LOCME("Updating all sources...") << std::endl;

            std::vector<Repository::SourceDetails> sources = Repository::GetSources();
            for (const auto& sd : sources)
            {
                out << LOCME("  Updating source: ") << sd.Name << "..." << std::flush;
                reporter.ExecuteWithProgress(std::bind(Repository::UpdateSource, sd.Name, std::placeholders::_1));
                out << LOCME(" Done.") << std::endl;
            }
        }
    }

    std::vector<Argument> SourceRemoveCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, LOCME("Name of the source to update"), ArgumentType::Positional, true },
        };
    }

    std::string SourceRemoveCommand::ShortDescription() const
    {
        return LOCME("Remove current sources");
    }

    std::vector<std::string> SourceRemoveCommand::GetLongDescription() const
    {
        return {
            LOCME("Remove current sources"),
        };
    }

    void SourceRemoveCommand::ExecuteInternal(Invocation& inv, std::ostream& out, std::istream& in) const
    {
        Workflow::WorkflowReporter reporter(out, in);

        const std::string& name = *inv.GetArg(s_SourceCommand_ArgName_Name);
        out << LOCME("Removing source: ") << name << "..." << std::endl;
        if (!reporter.ExecuteWithProgress(std::bind(Repository::RemoveSource, name, std::placeholders::_1)))
        {
            out << LOCME("Could not find a source by that name.") << std::endl;
        }
        else
        {
            out << LOCME("Done") << std::endl;
        }
    }
}
