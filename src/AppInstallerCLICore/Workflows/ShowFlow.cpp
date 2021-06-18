// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShowFlow.h"
#include "ManifestComparator.h"
#include "TableOutput.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    void ShowManifestInfo(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        const auto& installer = context.Get<Execution::Data::Installer>();

        // TODO: Come up with a prettier format
        context.Reporter.Info() << Execution::ManifestInfoEmphasis << "PackageVersion: " << manifest.Version << std::endl;
        context.Reporter.Info() << Execution::ManifestInfoEmphasis << "Publisher: " << manifest.CurrentLocalization.Get<Manifest::Localization::Publisher>() << std::endl;
        auto author = manifest.CurrentLocalization.Get<Manifest::Localization::Author>();
        if (!author.empty())
        {
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << "Author: " << author << std::endl;
        }
        if (!manifest.Moniker.empty())
        {
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << "Moniker: " << manifest.Moniker << std::endl;
        }
        auto description = manifest.CurrentLocalization.Get<Manifest::Localization::Description>();
        if (description.empty())
        {
            // Fall back to short description
            description = manifest.CurrentLocalization.Get<Manifest::Localization::ShortDescription>();
        }
        if (!description.empty())
        {
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << "Description: " << description << std::endl;
        }
        auto homepage = manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>();
        if (!homepage.empty())
        {
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << "PackageUrl: " << homepage << std::endl;
        }
        context.Reporter.Info() << Execution::ManifestInfoEmphasis << "License: " << manifest.CurrentLocalization.Get<Manifest::Localization::License>() << std::endl;
        auto licenseUrl = manifest.CurrentLocalization.Get<Manifest::Localization::LicenseUrl>();
        if (!licenseUrl.empty())
        {
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << "LicenseUrl: " << licenseUrl << std::endl;
        }

        context.Reporter.Info() << Execution::ManifestInfoEmphasis << "Installer:" << std::endl;
        if (installer)
        {
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << "  InstallerType: " << Manifest::InstallerTypeToString(installer->InstallerType) << std::endl;
            if (!installer->Locale.empty())
            {
                context.Reporter.Info() << Execution::ManifestInfoEmphasis << "  InstallerLocale: " << installer->Locale << std::endl;
            }
            if (!installer->Url.empty())
            {
                context.Reporter.Info() << Execution::ManifestInfoEmphasis << "  InstallerUrl: " << installer->Url << std::endl;
            }
            if (!installer->Sha256.empty())
            {
                context.Reporter.Info() << Execution::ManifestInfoEmphasis << "  InstallerSha256: " << Utility::SHA256::ConvertToString(installer->Sha256) << std::endl;
            }
            if (!installer->ProductId.empty())
            {
                context.Reporter.Info() << Execution::ManifestInfoEmphasis << "  ProductId: " << installer->ProductId << std::endl;
            }

            if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies)) {
                auto info = context.Reporter.Info();
                const auto& dependencies = installer->Dependencies;

                if (dependencies.HasAny())
                {
                    info << Execution::ManifestInfoEmphasis << "  Dependencies: " << std::endl;

                    info << "    - WindowsFeatures: ";
                    for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                        if (dep.Type == Manifest::DependencyType::WindowsFeature) info << "  " << dep << std::endl;
                    }

                    info << "    - WindowsLibraries: ";
                    for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                        if (dep.Type == Manifest::DependencyType::WindowsLibraries) info << "  " << dep << std::endl;
                    }

                    info << "    - PackageDependencies: ";
                    for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                        if (dep.Type == Manifest::DependencyType::Package)
                        {
                            info << "  " << dep.Id;
                            if (dep.MinVersion) info << " [>= " << dep.MinVersion << "]";
                            info << std::endl;
                        }
                    }

                    info << "    - ExternalDependencies: ";
                    for (const auto& dep : dependencies.dependencies) { //TODO change for lambda function called inside DepList
                        if (dep.Type == Manifest::DependencyType::External) info << "  " << dep << std::endl;
                    }
                }
            }
        }
        else
        {
            context.Reporter.Warn() << "  No installers are applicable to the current system" << std::endl;
        }
    }

    void ShowManifestVersion(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        Execution::TableOutput<2> table(context.Reporter, { Resource::String::ShowVersion, Resource::String::ShowChannel });
        table.OutputLine({ manifest.Version, manifest.Channel });
        table.Complete();
    }

    void ShowAppVersions(Execution::Context& context)
    {
        auto versions = context.Get<Execution::Data::Package>()->GetAvailableVersionKeys();

        Execution::TableOutput<2> table(context.Reporter, { Resource::String::ShowVersion, Resource::String::ShowChannel });
        for (const auto& version : versions)
        {
            table.OutputLine({ version.Version, version.Channel });
        }
        table.Complete();
    }
}