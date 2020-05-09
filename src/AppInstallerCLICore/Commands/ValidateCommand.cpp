// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ValidateCommand.h"
#include "Workflows/WorkflowBase.h"
#include "Resources.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    std::vector<Argument> ValidateCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ValidateManifest),
        };
    }

    Resource::LocString ValidateCommand::ShortDescription() const
    {
        return Resource::LocString{ Resource::String::ValidateCommandShortDescription };
    }

    Resource::LocString ValidateCommand::LongDescription() const
    {
        return Resource::LocString{ Resource::String::ValidateCommandLongDescription };
    }

    std::string ValidateCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-validate";
    }

    void ValidateCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::VerifyFile(Execution::Args::Type::ValidateManifest) <<
            [](Execution::Context& context)
        {
            auto inputFile = context.Args.GetArg(Execution::Args::Type::ValidateManifest);

            try
            {
                (void)Manifest::Manifest::CreateFromPath(inputFile, true, true);
                context.Reporter.Info() << "Manifest validation succeeded." << std::endl;
            }
            catch (const Manifest::ManifestException& e)
            {
                if (e.IsWarningOnly())
                {
                    context.Reporter.Warn() << "Manifest validation succeeded with warnings." << std::endl;
                }
                else
                {
                    context.Reporter.Error() << "Manifest validation failed." << std::endl;
                }

                context.Reporter.Info() << e.GetManifestErrorMessage() << std::endl;
            }
        };
    }
}
