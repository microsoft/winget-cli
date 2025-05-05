// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscCommand.h"
#include "DscPackageResource.h"
#include "DscUserSettingsFileResource.h"
#include "DscSourceResource.h"

#ifndef AICLI_DISABLE_TEST_HOOKS
#include "DscTestFileResource.h"
#include "DscTestJsonResource.h"
#endif

namespace AppInstaller::CLI
{
    namespace
    {
        Argument GetOutputFileArgument()
        {
            return { Execution::Args::Type::OutputFile, Resource::String::OutputFileArgumentDescription, ArgumentType::Standard };
        }
    }

    DscCommand::DscCommand(std::string_view parent) :
        Command(StaticName(), parent)
    {
    }

    std::vector<std::unique_ptr<Command>> DscCommand::GetCommands() const
    {
        // These should all derive from DscCommandBase
        return InitializeFromMoveOnly<std::vector<std::unique_ptr<Command>>>({
            std::make_unique<DscPackageResource>(FullName()),
            std::make_unique<DscSourceResource>(FullName()),
            std::make_unique<DscUserSettingsFileResource>(FullName()),
#ifndef AICLI_DISABLE_TEST_HOOKS
            std::make_unique<DscTestFileResource>(FullName()),
            std::make_unique<DscTestJsonResource>(FullName()),
#endif
        });
    }

    std::vector<Argument> DscCommand::GetArguments() const
    {
        std::vector<Argument> result;

        result.emplace_back(Execution::Args::Type::DscResourceFunctionManifest, Resource::String::DscResourceFunctionDescriptionManifest, ArgumentType::Flag);
        result.emplace_back(GetOutputFileArgument());

        return result;
    }

    Resource::LocString DscCommand::ShortDescription() const
    {
        return { Resource::String::DscCommandShortDescription };
    }

    Resource::LocString DscCommand::LongDescription() const
    {
        return { Resource::String::DscCommandLongDescription };
    }

    Utility::LocIndView DscCommand::HelpLink() const
    {
        return "https://aka.ms/winget-dsc-resources"_liv;
    }

    void DscCommand::ExecuteInternal(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::DscResourceFunctionManifest))
        {
            std::filesystem::path outputDirectory{ Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::OutputFile)) };
            std::filesystem::create_directories(outputDirectory);

            std::string filePrefix = Utility::ToLower(DscCommandBase::ModuleName());

            for (const auto& command : GetCommands())
            {
                DscCommandBase* commandBase = static_cast<DscCommandBase*>(command.get());

                std::filesystem::path outputPath = outputDirectory;
                outputPath /= std::string{ filePrefix }.append(".").append(commandBase->Name()).append(".dsc.resource.json");
                commandBase->WriteManifest(context, outputPath);
            }
        }
        else
        {
            OutputHelp(context.Reporter);
        }
    }

    void DscCommand::ValidateArgumentsInternal(Execution::Args& args) const
    {
        if (args.Contains(Execution::Args::Type::DscResourceFunctionManifest) &&
            !args.Contains(Execution::Args::Type::OutputFile))
        {
            throw CommandException(Resource::String::RequiredArgError(GetOutputFileArgument().Name()));
        }
    }
}
