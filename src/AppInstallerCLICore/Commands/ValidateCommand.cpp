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

    std::string ValidateCommand::ShortDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"ValidateCommandShortDescription");
    }

    std::string ValidateCommand::GetLongDescription() const
    {
        return Resources::GetInstance().ResolveWingetString(L"ValidateCommandLongDescription");
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
