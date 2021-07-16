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

                    if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsFeature))
                    {
                        info << "    - WindowsFeatures: " << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::WindowsFeature, [&info](Manifest::Dependency dependency) {info << "        " << dependency.Id << std::endl; });
                    }

                    if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsLibrary))
                    {
                        info << "    - WindowsLibraries: " << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::WindowsLibrary, [&info](Manifest::Dependency dependency) {info << "        " << dependency.Id << std::endl; });
                    }

                    if (dependencies.HasAnyOf(Manifest::DependencyType::Package))
                    {
                        info << "    - PackageDependencies: " << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::Package, [&info](Manifest::Dependency dependency) {
                            info << "        " << dependency.Id;
                            if (dependency.MinVersion) info << " [>= " << dependency.MinVersion.value() << "]";
                            info << std::endl;
                        });
                    }

                    if (dependencies.HasAnyOf(Manifest::DependencyType::External))
                    {
                        info << "    - ExternalDependencies: " << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::External, [&info](Manifest::Dependency dependency) {info << "        " << dependency.Id << std::endl; });
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