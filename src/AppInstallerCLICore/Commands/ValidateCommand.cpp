// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ValidateCommand.h"
#include "Localization.h"
#include "Workflows/WorkflowBase.h"

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;

    std::vector<Argument> ValidateCommand::GetArguments() const
    {
        return {
            Argument::ForType(Execution::Args::Type::ValidateManifest),
        };
    }

    std::string ValidateCommand::ShortDescription() const
    {
        return LOCME("Validates a manifest file");
    }

    std::string ValidateCommand::GetLongDescription() const
    {
        return LOCME("Validates a manifest using a strict set of guidelines. This is intended to enable you to check your manifest before submitting to a repo.");
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
                    context.Reporter.Info() << "Manifest validation succeeded with warnings." << std::endl;
                }
                else
                {
                    context.Reporter.Warn() << "Manifest validation failed." << std::endl;
                }

                context.Reporter.Warn() << e.GetManifestErrorMessage() << std::endl;
            }
        };
    }
}
