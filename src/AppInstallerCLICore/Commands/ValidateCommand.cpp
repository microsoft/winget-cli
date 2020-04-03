// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ValidateCommand.h"
#include "Localization.h"

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
        auto inputFile = context.Args.GetArg(Execution::Args::Type::ValidateManifest);

        if (!std::filesystem::exists(inputFile))
        {
            AICLI_LOG(CLI, Error, << "Input file does not exist. Path: " << inputFile);
            context.Reporter.Error() << "The input manifest file does not exist. Path: " << inputFile << std::endl;
            return;
        }

        try
        {
            Manifest::Manifest::CreateFromPath(inputFile, true);
            context.Reporter.Info() << "Manifest validation succeeded." << std::endl;
        }
        catch (const Manifest::ManifestException& e)
        {
            context.Reporter.Warn() << "Manifest validation failed." << std::endl;
            context.Reporter.Warn() << e.GetManifestErrorMessage() << std::endl;
        }
    }
}
