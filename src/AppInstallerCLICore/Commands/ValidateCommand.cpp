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

    void ShowWindowsFeatureDependencies(std::vector<AppInstaller::Manifest::string_t>& windowsFeaturesDep, AppInstaller::CLI::Execution::Context& context)
    {
        if (!windowsFeaturesDep.empty())
        {
            context.Reporter.Info() << "    - Windows Features: ";
            for (size_t i = 0; i < windowsFeaturesDep.size(); i++)
            {
                context.Reporter.Info() << windowsFeaturesDep[i];
                if (i < windowsFeaturesDep.size() - 1) context.Reporter.Info() << ", ";
            }
            context.Reporter.Info() << std::endl;
        }
    }

    void ShowWindowsLibrariesDependencies(std::vector<AppInstaller::Manifest::string_t>& windowsLibrariesDep, AppInstaller::CLI::Execution::Context& context)
    {
        if (!windowsLibrariesDep.empty())
        {
            context.Reporter.Info() << "    - Windows Libraries: ";
            for (size_t i = 0; i < windowsLibrariesDep.size(); i++)
            {
                context.Reporter.Info() << windowsLibrariesDep[i];
                if (i < windowsLibrariesDep.size() - 1) context.Reporter.Info() << ", ";
            }
            context.Reporter.Info() << std::endl;
        }
    }

    void ShowPackageDependencies(std::vector<AppInstaller::Manifest::PackageDependency>& packageDep, AppInstaller::CLI::Execution::Context& context)
    {
        if (!packageDep.empty())
        {
            context.Reporter.Info() << "    - Packages: ";
            for (size_t i = 0; i < packageDep.size(); i++)
            {
                context.Reporter.Info() << packageDep[i].Id;
                if (!packageDep[i].MinVersion.empty()) context.Reporter.Info() << " [>= " << packageDep[i].MinVersion << "]";
                if (i < packageDep.size() - 1) context.Reporter.Info() << ", ";
            }
            context.Reporter.Info() << std::endl;
        }
    }

    void ShowExternalDependencies(std::vector<AppInstaller::Manifest::string_t>& externalDependenciesDep, AppInstaller::CLI::Execution::Context& context)
    {
        if (!externalDependenciesDep.empty())
        {
            context.Reporter.Info() << "    - Externals: ";
            for (size_t i = 0; i < externalDependenciesDep.size(); i++)
            {
                context.Reporter.Info() << externalDependenciesDep[i];
                if (i < externalDependenciesDep.size() - 1) context.Reporter.Info() << ", ";
            }
            context.Reporter.Info() << std::endl;
        }
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
                auto manifest = Manifest::YamlParser::CreateFromPath(inputFile, true, true);

                if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::EFExperimentalShowDependencies)) 
                {
                    std::vector<AppInstaller::Manifest::string_t> windowsFeaturesDep;
                    std::vector<AppInstaller::Manifest::string_t> windowsLibrariesDep;
                    std::vector<AppInstaller::Manifest::PackageDependency> packageDep;
                    std::vector<AppInstaller::Manifest::string_t> externalDep;
                    for (auto installer : manifest.Installers)
                    {
                        auto dependencies = installer.Dependencies;
                        windowsFeaturesDep.insert(windowsFeaturesDep.begin(),
                            dependencies.WindowsFeatures.begin(), dependencies.WindowsFeatures.end());
                        windowsLibrariesDep.insert(windowsLibrariesDep.begin(),
                            dependencies.WindowsLibraries.begin(), dependencies.WindowsLibraries.end());
                        packageDep.insert(packageDep.begin(),
                            dependencies.PackageDependencies.begin(), dependencies.PackageDependencies.end());
                        externalDep.insert(externalDep.begin(),
                            dependencies.ExternalDependencies.begin(), dependencies.ExternalDependencies.end());
                    }

                    context.Reporter.Info() << "Manifest has the following dependencies:" << std::endl;
                    ShowWindowsFeatureDependencies(windowsFeaturesDep, context);
                    ShowWindowsLibrariesDependencies(windowsLibrariesDep, context);
                    ShowPackageDependencies(packageDep, context);
                    ShowExternalDependencies(externalDep, context);
                }

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
