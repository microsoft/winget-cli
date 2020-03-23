// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SourceCommand.h"
#include "Localization.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    constexpr std::string_view s_SourceCommand_ArgName_Name = "name"sv;
    constexpr std::string_view s_SourceCommand_ArgName_Type = "type"sv;
    constexpr std::string_view s_SourceCommand_ArgName_Arg = "arg"sv;

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

    void SourceCommand::ExecuteInternal(Execution::Context& context) const
    {
        OutputHelp(context.Reporter);
    }

    std::vector<Argument> SourceAddCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, Execution::Args::Type::SourceName, LOCME("Name of the source for future reference"), ArgumentType::Positional, true },
            Argument{ s_SourceCommand_ArgName_Arg, Execution::Args::Type::SourceArg, LOCME("Argument given to the source"), ArgumentType::Positional, true },
            Argument{ s_SourceCommand_ArgName_Type, Execution::Args::Type::SourceType, LOCME("Type of the source"), ArgumentType::Positional, false },
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

    void SourceAddCommand::ExecuteInternal(Execution::Context& context) const
    {
        std::string name = *context.Args.GetArg(Execution::Args::Type::SourceName);
        std::string arg = *context.Args.GetArg(Execution::Args::Type::SourceArg);
        std::string type;
        if (context.Args.Contains(Execution::Args::Type::SourceType))
        {
            type = *context.Args.GetArg(Execution::Args::Type::SourceType);
        }

        context.Reporter.ShowMsg("Adding source:");
        context.Reporter.ShowMsg("  Name: " + name);
        context.Reporter.ShowMsg("  Arg: " + arg);
        if (!type.empty())
        {
            context.Reporter.ShowMsg("  Type: " + type);
        }

        context.Reporter.ExecuteWithProgress(std::bind(Repository::AddSource, std::move(name), std::move(type), std::move(arg), std::placeholders::_1));

        context.Reporter.ShowMsg("Done");
    }

    std::vector<Argument> SourceListCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, Execution::Args::Type::SourceName, LOCME("Name of the source to list full details for"), ArgumentType::Positional, false },
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

    void SourceListCommand::ExecuteInternal(Execution::Context& context) const
    {
        std::vector<Repository::SourceDetails> sources = Repository::GetSources();

        if (context.Args.Contains(Execution::Args::Type::SourceName))
        {
            const std::string& name = *context.Args.GetArg(Execution::Args::Type::SourceName);
            auto itr = std::find_if(sources.begin(), sources.end(), [name](const Repository::SourceDetails& sd) { return Utility::CaseInsensitiveEquals(sd.Name, name); });

            if (itr == sources.end())
            {
                context.Reporter.ShowMsg("No source with the given name was found: " + name);
            }
            else
            {
                context.Reporter.ShowMsg("Name: " + itr->Name);
                context.Reporter.ShowMsg("Type: " + itr->Type);
                context.Reporter.ShowMsg("Arg: " + itr->Arg);
                context.Reporter.ShowMsg("Data: " + itr->Data);
                if (itr->LastUpdateTime == Utility::ConvertUnixEpochToSystemClock(0))
                {
                    context.Reporter.ShowMsg("Last Update: <never>");
                }
                else
                {
                    std::stringstream stream;
                    stream << itr->LastUpdateTime;
                    context.Reporter.ShowMsg("Last Update: " + stream.str());
                }
            }
        }
        else
        {
            context.Reporter.ShowMsg("Current sources:");

            if (sources.empty())
            {
                context.Reporter.ShowMsg("  <none>");
            }
            else
            {
                for (const auto& source : sources)
                {
                    context.Reporter.ShowMsg("  " + source.Name + " => " + source.Arg);
                }
            }
        }
    }

    std::vector<Argument> SourceUpdateCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, Execution::Args::Type::SourceName, LOCME("Name of the source to update"), ArgumentType::Positional, false },
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

    void SourceUpdateCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::SourceName))
        {
            const std::string& name = *context.Args.GetArg(Execution::Args::Type::SourceName);
            context.Reporter.ShowMsg("Updating source: " + name + "...");
            if (!context.Reporter.ExecuteWithProgress(std::bind(Repository::UpdateSource, name, std::placeholders::_1)))
            {
                context.Reporter.EmptyLine();
                context.Reporter.ShowMsg("  Could not find a source by that name.", Execution::Reporter::Level::Warning);
            }
            else
            {
                context.Reporter.ShowMsg("Done");
            }
        }
        else
        {
            context.Reporter.ShowMsg("Updating all sources...");

            std::vector<Repository::SourceDetails> sources = Repository::GetSources();
            for (const auto& sd : sources)
            {
                context.Reporter.ShowMsg("Updating source: " + sd.Name + "...");
                context.Reporter.ExecuteWithProgress(std::bind(Repository::UpdateSource, sd.Name, std::placeholders::_1));
                context.Reporter.ShowMsg(LOCME("Done.") );
            }
        }
    }

    std::vector<Argument> SourceRemoveCommand::GetArguments() const
    {
        return {
            Argument{ s_SourceCommand_ArgName_Name, Execution::Args::Type::SourceName, LOCME("Name of the source to remove"), ArgumentType::Positional, true },
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

    void SourceRemoveCommand::ExecuteInternal(Execution::Context& context) const
    {
        const std::string& name = *context.Args.GetArg(Execution::Args::Type::SourceName);
        context.Reporter.ShowMsg("Removing source: " + name + "...");
        if (!context.Reporter.ExecuteWithProgress(std::bind(Repository::RemoveSource, name, std::placeholders::_1)))
        {
            context.Reporter.ShowMsg("Could not find a source by that name.", Execution::Reporter::Level::Warning);
        }
        else
        {
            context.Reporter.ShowMsg("Done");
        }
    }
}
