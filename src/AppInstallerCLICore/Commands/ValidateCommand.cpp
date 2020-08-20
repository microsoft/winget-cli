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
            auto inputFile = Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::ValidateManifest));

            try
            {
                (void)Manifest::YamlParser::CreateFromPath(inputFile, true, true);
                context.Reporter.Info() << Resource::String::ManifestValidationSuccess << std::endl;
            }
            catch (const Manifest::ManifestException& e)
            {
                HRESULT hr = S_OK;
                if (e.IsWarningOnly())
                {
                    context.Reporter.Warn() << Resource::String::ManifestValidationWarning << std::endl;
                    hr = APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_WARNING;
                }
                else
                {
                    context.Reporter.Error() << Resource::String::ManifestValidationFail << std::endl;
                    hr = APPINSTALLER_CLI_ERROR_MANIFEST_VALIDATION_FAILURE;
                }

                context.Reporter.Info() << e.GetManifestErrorMessage() << std::endl;
                AICLI_TERMINATE_CONTEXT(hr);
            }
        };
    }
}
