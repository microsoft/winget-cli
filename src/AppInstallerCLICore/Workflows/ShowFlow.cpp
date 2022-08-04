// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShowFlow.h"
#include "ManifestComparator.h"
#include "TableOutput.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;

namespace {

    template <class CharT, typename String>
    void ShowSingleLineField(Execution::OutputStream& outputStream, CharT label, String value, bool indent = false)
    {
        if (value.empty())
        {
            return;
        }
        if (indent)
        {
            outputStream << "  "_liv;
        }
        outputStream << Execution::ManifestInfoEmphasis << label << ' ' << value << std::endl;
    }

    template <class CharT, typename String>
    void ShowMultiLineField(Execution::OutputStream& outputStream, CharT label, String value)
    {
        if (value.empty())
        {
            return;
        }
        bool isMultiLine = FindAndReplace(value, "\n", "\n  ");
        outputStream << Execution::ManifestInfoEmphasis << label;
        if (isMultiLine)
        {
            outputStream << std::endl << "  "_liv << value << std::endl;
        }
        else
        {
            outputStream << ' ' << value << std::endl;
        }
    }

    template <typename Enumerable>
    void ShowMultiValueField(Execution::OutputStream& outputStream, AppInstaller::StringResource::StringId label, Enumerable values)
    {
        if (values.empty())
        {
            return;
        }
        outputStream << Execution::ManifestInfoEmphasis << label << std::endl;
        for (const auto& value : values)
        {
            outputStream << "  "_liv << value << std::endl;
        }
    }
}

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
        // Get description from manifest so we can see if it is empty later
        auto description = manifest.CurrentLocalization.Get<Manifest::Localization::Description>();

        // TODO: Come up with a prettier format
        ShowSingleLineField(info, Resource::String::ShowLabelVersion, manifest.Version);
        ShowSingleLineField(info, Resource::String::ShowLabelPublisher, manifest.CurrentLocalization.Get<Manifest::Localization::Publisher>());
        ShowSingleLineField(info, Resource::String::ShowLabelPublisherUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PublisherUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelPublisherSupportUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PublisherSupportUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelAuthor, manifest.CurrentLocalization.Get<Manifest::Localization::Author>());
        ShowSingleLineField(info, Resource::String::ShowLabelMoniker, manifest.Moniker);
        ShowMultiLineField(info,  Resource::String::ShowLabelDescription, description.empty() ? manifest.CurrentLocalization.Get<Manifest::Localization::ShortDescription>() : description);
        ShowSingleLineField(info, Resource::String::ShowLabelPackageUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelLicense, manifest.CurrentLocalization.Get<Manifest::Localization::License>());
        ShowSingleLineField(info, Resource::String::ShowLabelLicenseUrl, manifest.CurrentLocalization.Get<Manifest::Localization::LicenseUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelPrivacyUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PrivacyUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelCopyright, manifest.CurrentLocalization.Get<Manifest::Localization::Copyright>());
        ShowSingleLineField(info, Resource::String::ShowLabelCopyrightUrl, manifest.CurrentLocalization.Get<Manifest::Localization::CopyrightUrl>());
        ShowMultiLineField(info,  Resource::String::ShowLabelReleaseNotes, manifest.CurrentLocalization.Get<Manifest::Localization::ReleaseNotes>());
        ShowSingleLineField(info, Resource::String::ShowLabelReleaseNotesUrl, manifest.CurrentLocalization.Get<Manifest::Localization::ReleaseNotesUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelPurchaseUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PurchaseUrl>());
        ShowMultiLineField(info, Resource::String::ShowLabelInstallationNotes, manifest.CurrentLocalization.Get<Manifest::Localization::InstallationNotes>());
        const auto& documentations = manifest.CurrentLocalization.Get<Manifest::Localization::Documentations>();
        if (!documentations.empty())
        {
            ShowSingleLineField(info, Resource::String::ShowLabelDocumentation, " "_liv);
            for (const auto& documentation : documentations)
            {
                if (!documentation.DocumentUrl.empty())
                {
                    info << "  "_liv;
                    if (!documentation.DocumentLabel.empty())
                    {
                       info << Execution::ManifestInfoEmphasis << documentation.DocumentLabel << ": "_liv;
                    }
                    info << documentation.DocumentUrl << std::endl;
                }
            }
        }
        ShowMultiValueField(info, Resource::String::ShowLabelTags, manifest.CurrentLocalization.Get<Manifest::Localization::Tags>());
        const auto& agreements = manifest.CurrentLocalization.Get<Manifest::Localization::Agreements>();
        if (!agreements.empty())
        {
            ShowSingleLineField(info, Resource::String::ShowLabelAgreements, " "_liv);
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
            Manifest::InstallerTypeEnum effectiveInstallerType = installer->EffectiveInstallerType();
            Manifest::InstallerTypeEnum baseInstallerType = installer->BaseInstallerType;
            if (effectiveInstallerType == baseInstallerType)
            {
                info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerType << ' ' << Manifest::InstallerTypeToString(effectiveInstallerType) << std::endl;
            }
            else
            {
                info << "  "_liv << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelInstallerType << ' ' << Manifest::InstallerTypeToString(effectiveInstallerType) << " (" <<
                    Manifest::InstallerTypeToString(baseInstallerType) << ')' << std::endl;
            }
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerLocale, installer->Locale, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerUrl, installer->Url, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerSha256, Utility::SHA256::ConvertToString(installer->Sha256), true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerProductId, installer->ProductId, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerReleaseDate, installer->ReleaseDate, true);

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
                                {
                                    info << " [>= " << dependency.MinVersion.value().ToString() << "]";
                                }
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