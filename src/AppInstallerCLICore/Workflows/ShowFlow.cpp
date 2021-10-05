// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShowFlow.h"
#include "ManifestComparator.h"
#include "TableOutput.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Workflow
{
    void ShowManifestInfo(Execution::Context& context)
    {
        context << ShowPackageInfo << ShowInstallerInfo;
    }

    void ShowPackageInfo(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        auto info = context.Reporter.Info();

        // TODO: Come up with a prettier format
        info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelVersion << ' ' << manifest.Version << std::endl;
        info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelPublisher << ' ' << manifest.CurrentLocalization.Get<Manifest::Localization::Publisher>() << std::endl;
        auto publisherUrl = manifest.CurrentLocalization.Get<Manifest::Localization::PublisherUrl>();
        if (!publisherUrl.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelPublisherUrl << ' ' << publisherUrl << std::endl;
        }
        auto publisherSupportUrl = manifest.CurrentLocalization.Get<Manifest::Localization::PublisherSupportUrl>();
        if (!publisherSupportUrl.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelPublisherSupportUrl << ' ' << publisherSupportUrl << std::endl;
        }
        auto author = manifest.CurrentLocalization.Get<Manifest::Localization::Author>();
        if (!author.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelAuthor << ' ' << author << std::endl;
        }
        if (!manifest.Moniker.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelMoniker << ' ' << manifest.Moniker << std::endl;
        }
        auto description = manifest.CurrentLocalization.Get<Manifest::Localization::Description>();
        if (description.empty())
        {
            // Fall back to short description
            description = manifest.CurrentLocalization.Get<Manifest::Localization::ShortDescription>();
        }
        if (!description.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelDescription << ' ' << description << std::endl;
        }
        auto homepage = manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>();
        if (!homepage.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelPackageUrl << ' ' << homepage << std::endl;
        }
        info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelLicense << ' ' << manifest.CurrentLocalization.Get<Manifest::Localization::License>() << std::endl;
        auto licenseUrl = manifest.CurrentLocalization.Get<Manifest::Localization::LicenseUrl>();
        if (!licenseUrl.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelLicenseUrl << ' ' << licenseUrl << std::endl;
        }
        auto privacyUrl = manifest.CurrentLocalization.Get<Manifest::Localization::PrivacyUrl>();
        if (!privacyUrl.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelPrivacyUrl << ' ' << privacyUrl << std::endl;
        }
        auto copyright = manifest.CurrentLocalization.Get<Manifest::Localization::Copyright>();
        if (!copyright.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelCopyright << ' ' << copyright << std::endl;
        }
        auto copyrightUrl = manifest.CurrentLocalization.Get<Manifest::Localization::CopyrightUrl>();
        if (!copyrightUrl.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelCopyrightUrl << ' ' << copyrightUrl << std::endl;
        }
        auto releaseNotes = manifest.CurrentLocalization.Get<Manifest::Localization::ReleaseNotes>();
        if (!releaseNotes.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelReleaseNotes << ' ' << releaseNotes << std::endl;
        }
        auto releaseNotesUrl = manifest.CurrentLocalization.Get<Manifest::Localization::ReleaseNotesUrl>();
        if (!releaseNotesUrl.empty())
        {
            info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelReleaseNotesUrl << ' ' << releaseNotesUrl << std::endl;
        }
        auto agreements = manifest.CurrentLocalization.Get<Manifest::Localization::Agreements>();
        if (!agreements.empty())
        {
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelAgreements << std::endl;
            for (const auto& agreement : agreements)
            {
                if (!agreement.Label.empty())
                {
                    info << Execution::ManifestInfoEmphasis << agreement.Label << ": "_liv;
                }

                if (!agreement.AgreementText.empty())
                {
                    info << agreement.AgreementText << std::endl;
                }

                if (!agreement.AgreementUrl.empty())
                {
                    info << agreement.AgreementUrl << std::endl;
                }
            }

            info << std::endl;
        }
    }

    void ShowInstallerInfo(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();
        auto info = context.Reporter.Info();

        info << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstaller << std::endl;
        if (installer)
        {
            info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerType << ' ' << Manifest::InstallerTypeToString(installer->InstallerType) << std::endl;
            if (!installer->Locale.empty())
            {
                info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerLocale << ' ' << installer->Locale << std::endl;
            }
            if (!installer->Url.empty())
            {
                info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerUrl << ' ' << installer->Url << std::endl;
            }
            if (!installer->Sha256.empty())
            {
                info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerSha256 << ' ' << Utility::SHA256::ConvertToString(installer->Sha256) << std::endl;
            }
            if (!installer->ProductId.empty())
            {
                info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerProductId << ' ' << installer->ProductId << std::endl;
            }
            if (!installer->ReleaseDate.empty())
            {
                info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerReleaseDate << ' ' << installer->ReleaseDate << std::endl;
            }

            if (Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Dependencies))
            {
                const auto& dependencies = installer->Dependencies;

                if (dependencies.HasAny())
                {
                    info << Execution::ManifestInfoEmphasis << "  "_liv << Resource::String::ShowLabelDependencies << ' ' << std::endl;

                    if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsFeature))
                    {
                        info << "    - "_liv << Resource::String::ShowLabelWindowsFeaturesDependencies << ' ' << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::WindowsFeature, [&info](Manifest::Dependency dependency) {info << "        "_liv << dependency.Id << std::endl; });
                    }

                    if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsLibrary))
                    {
                        info << "    - "_liv << Resource::String::ShowLabelWindowsLibrariesDependencies << ' ' << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::WindowsLibrary, [&info](Manifest::Dependency dependency) {info << "        "_liv << dependency.Id << std::endl; });
                    }

                    if (dependencies.HasAnyOf(Manifest::DependencyType::Package))
                    {
                        info << "    - "_liv << Resource::String::ShowLabelPackageDependencies << ' ' << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::Package, [&info](Manifest::Dependency dependency)
                            {
                                info << "        "_liv << dependency.Id;
                                if (dependency.MinVersion) 
                                   info << " [>= " << dependency.MinVersion.value().ToString() << "]";
                                info << std::endl;
                            });
                    }

                    if (dependencies.HasAnyOf(Manifest::DependencyType::External))
                    {
                        info << "    - "_liv << Resource::String::ShowLabelExternalDependencies << ' ' << std::endl;
                        dependencies.ApplyToType(Manifest::DependencyType::External, [&info](Manifest::Dependency dependency) {info << "        "_liv << dependency.Id << std::endl; });
                    }
                }
            }
        }
        else
        {
            context.Reporter.Warn() << "  "_liv << Resource::String::NoApplicableInstallers << std::endl;
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