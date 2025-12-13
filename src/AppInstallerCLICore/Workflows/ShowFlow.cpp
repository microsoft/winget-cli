// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShowFlow.h"
#include <winget/ManifestComparator.h>
#include "TableOutput.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::CLI;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Workflow
{
    namespace {
        LocIndView GetIndentFor(size_t indentLevel)
        {
            static constexpr std::array<LocIndView, 4> s_indents{ ""_liv, "  "_liv, "    "_liv, "      "_liv };
            return s_indents.at(indentLevel);
        }

        void ShowSingleLineField(Execution::OutputStream& outputStream, StringResource::StringId label, const Manifest::Manifest::string_t& value, bool indent = false)
        {
            Workflow::ShowSingleLineField(outputStream, label, LocIndView{ value }, indent ? 1 : 0);
        }

        void ShowMultiLineField(Execution::OutputStream& outputStream, StringResource::StringId label, const Manifest::Manifest::string_t& value)
        {
            Workflow::ShowMultiLineField(outputStream, label, LocIndView{ value });
        }

        void ShowMultiValueField(Execution::OutputStream& outputStream, StringResource::StringId label, const std::vector<Manifest::Manifest::string_t>& values)
        {
            Workflow::ShowMultiValueField(outputStream, label, Enumerable<LocIndString>{ values, [](const Manifest::Manifest::string_t& s) { return LocIndString{ s }; } });
        }

        void ShowAgreements(Execution::OutputStream& outputStream, const std::vector<Manifest::Agreement>& agreements) {

            if (agreements.empty()) {
                return;
            }

            outputStream << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelAgreements << std::endl;
            for (const auto& agreement : agreements) {

                if (!agreement.Label.empty())
                {
                    outputStream << "  "_liv << Execution::ManifestInfoEmphasis << agreement.Label << ": "_liv;
                }

                if (!agreement.AgreementText.empty())
                {
                    outputStream << agreement.AgreementText << std::endl;
                }

                if (!agreement.AgreementUrl.empty())
                {
                    outputStream << agreement.AgreementUrl << std::endl;
                }
            }
        }
    }

    void ShowAgreementsInfo(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        auto info = context.Reporter.Info();

        ShowSingleLineField(info, Resource::String::ShowLabelVersion, manifest.Version);
        ShowSingleLineField(info, Resource::String::ShowLabelPublisher, manifest.CurrentLocalization.Get<Manifest::Localization::Publisher>());
        ShowSingleLineField(info, Resource::String::ShowLabelPublisherUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PublisherUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelPublisherSupportUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PublisherSupportUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelAuthor, manifest.CurrentLocalization.Get<Manifest::Localization::Author>());
        ShowSingleLineField(info, Resource::String::ShowLabelPackageUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelLicense, manifest.CurrentLocalization.Get<Manifest::Localization::License>());
        ShowSingleLineField(info, Resource::String::ShowLabelLicenseUrl, manifest.CurrentLocalization.Get<Manifest::Localization::LicenseUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelPrivacyUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PrivacyUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelCopyright, manifest.CurrentLocalization.Get<Manifest::Localization::Copyright>());
        ShowSingleLineField(info, Resource::String::ShowLabelCopyrightUrl, manifest.CurrentLocalization.Get<Manifest::Localization::CopyrightUrl>());
        ShowSingleLineField(info, Resource::String::ShowLabelPurchaseUrl, manifest.CurrentLocalization.Get<Manifest::Localization::PurchaseUrl>());
        ShowAgreements(info, manifest.CurrentLocalization.Get<Manifest::Localization::Agreements>());
        
    }

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
            context.Reporter.Info() << Execution::ManifestInfoEmphasis << Resource::String::ShowLabelDocumentation << std::endl;
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
        ShowAgreements(info, manifest.CurrentLocalization.Get<Manifest::Localization::Agreements>());
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
            std::string shownInstallerType;
            shownInstallerType = Manifest::InstallerTypeToString(effectiveInstallerType);
            if (effectiveInstallerType != baseInstallerType)
            {
                shownInstallerType += " ("_liv;
                shownInstallerType += Manifest::InstallerTypeToString(baseInstallerType);
                shownInstallerType += ')';
            }
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerType, shownInstallerType, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerLocale, installer->Locale, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerUrl, installer->Url, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerSha256, (installer->Sha256.empty()) ? "" : Utility::SHA256::ConvertToString(installer->Sha256), true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerProductId, installer->ProductId, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerReleaseDate, installer->ReleaseDate, true);
            ShowSingleLineField(info, Resource::String::ShowLabelInstallerOfflineDistributionSupported, Utility::ConvertBoolToString(!installer->DownloadCommandProhibited), true);

            const auto& dependencies = installer->Dependencies;

            if (dependencies.HasAny())
            {
                info << Execution::ManifestInfoEmphasis << "  "_liv << Resource::String::ShowLabelDependencies << ' ' << std::endl;

                if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsFeature))
                {
                    info << "    - "_liv << Resource::String::ShowLabelWindowsFeaturesDependencies << ' ' << std::endl;
                    dependencies.ApplyToType(Manifest::DependencyType::WindowsFeature, [&info](Manifest::Dependency dependency) {info << "        "_liv << dependency.Id() << std::endl; });
                }

                if (dependencies.HasAnyOf(Manifest::DependencyType::WindowsLibrary))
                {
                    info << "    - "_liv << Resource::String::ShowLabelWindowsLibrariesDependencies << ' ' << std::endl;
                    dependencies.ApplyToType(Manifest::DependencyType::WindowsLibrary, [&info](Manifest::Dependency dependency) {info << "        "_liv << dependency.Id() << std::endl; });
                }

                if (dependencies.HasAnyOf(Manifest::DependencyType::Package))
                {
                    info << "    - "_liv << Resource::String::ShowLabelPackageDependencies << ' ' << std::endl;
                    dependencies.ApplyToType(Manifest::DependencyType::Package, [&info](Manifest::Dependency dependency)
                        {
                            info << "        "_liv << dependency.Id();
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
                    dependencies.ApplyToType(Manifest::DependencyType::External, [&info](Manifest::Dependency dependency) {info << "        "_liv << dependency.Id() << std::endl; });
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

    void GetManifest::operator()(Execution::Context& context) const
    {
        if (context.Args.Contains(Execution::Args::Type::Manifest))
        {
            context <<
                GetManifestFromArg;
        }
        else
        {
            context <<
                OpenSource() <<
                SearchSourceForSingle <<
                HandleSearchResultFailures <<
                EnsureOneMatchFromSearchResult(OperationType::Show) <<
                GetManifestFromPackage(m_considerPins);
        }
    }

    void ShowSingleLineField(Execution::OutputStream& outputStream, StringResource::StringId label, LocIndView value, size_t indentLevel)
    {
        if (value.empty())
        {
            return;
        }

        outputStream << GetIndentFor(indentLevel) << Execution::ManifestInfoEmphasis << label << ' ' << value << '\n';
    }

    void ShowMultiLineField(Execution::OutputStream& outputStream, StringResource::StringId label, LocIndView value, size_t indentLevel)
    {
        if (value.empty())
        {
            return;
        }

        auto lines = Split(value, '\n');

        outputStream << GetIndentFor(indentLevel) << Execution::ManifestInfoEmphasis << label;

        if (lines.size() > 1)
        {
            for (const auto& line : lines)
            {
                outputStream << '\n' << GetIndentFor(indentLevel + 1) << line << '\n';
            }
        }
        else
        {
            outputStream << ' ' << value << '\n';
        }
    }

    void ShowMultiValueField(Execution::OutputStream& outputStream, StringResource::StringId label, Enumerable<Utility::LocIndString> values, size_t indentLevel)
    {
        if (values.AtEnd())
        {
            return;
        }

        outputStream << GetIndentFor(indentLevel) << Execution::ManifestInfoEmphasis << label << '\n';

        do
        {
            outputStream << GetIndentFor(indentLevel + 1) << values.Current() << '\n';
        } while (values.Next());
    }
}
