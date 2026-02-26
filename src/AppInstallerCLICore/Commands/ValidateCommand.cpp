// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ValidateCommand.h"
#include "Workflows/WorkflowBase.h"
#include "Workflows/DependenciesFlow.h"
#include "Resources.h"
#include <winget/ManifestYamlParser.h>

namespace AppInstaller::CLI
{
    using namespace std::string_view_literals;
    using namespace AppInstaller::Manifest;

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

    Utility::LocIndView ValidateCommand::HelpLink() const
    {
        return "https://aka.ms/winget-command-validate"_liv;
    }

    void ValidateCommand::ExecuteInternal(Execution::Context& context) const
    {
        context <<
            Workflow::VerifyPath(Execution::Args::Type::ValidateManifest) <<
            [](Execution::Context& context)
        {
            auto inputFile = Utility::ConvertToUTF16(context.Args.GetArg(Execution::Args::Type::ValidateManifest));

            try
            {
                ManifestValidateOption validateOption;
                validateOption.FullValidation = true;
                validateOption.SchemaHeaderValidationAsWarning = true;
                validateOption.ThrowOnWarning = !(context.Args.Contains(Execution::Args::Type::IgnoreWarnings));
                auto manifest = YamlParser::CreateFromPath(inputFile, validateOption);

                context.Add<Execution::Data::Manifest>(manifest);
                context <<
                    Workflow::GetInstallersDependenciesFromManifest <<
                    Workflow::ReportDependencies(Resource::String::ValidateCommandReportDependencies);

                context.Reporter.Info() << Resource::String::ManifestValidationSuccess << std::endl;
            }
            catch (const ManifestException& e)
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
