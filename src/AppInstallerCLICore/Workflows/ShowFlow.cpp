// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShowFlow.h"
#include "ManifestComparator.h"
#include "TableOutput.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::CLI;

namespace AppInstaller::CLI::Workflow
{
    void ShowManifestInfo(Execution::Context& context)
    {
        context << ShowManifestInfoOnly << ShowInstallerInfo;
    }

    void ShowManifestInfoOnly(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        auto info = context.Reporter.Info();

        // TODO: Come up with a prettier format
        info << Execution::LabelEmphasis << Resource::String::ShowLabelVersion << " " << manifest.Version << std::endl;
        info << Execution::LabelEmphasis << Resource::String::ShowLabelPublisher << " " << manifest.CurrentLocalization.Get<Manifest::Localization::Publisher>() << std::endl;
        auto author = manifest.CurrentLocalization.Get<Manifest::Localization::Author>();
        if (!author.empty())
        {
            info << Execution::LabelEmphasis << Resource::String::ShowLabelAuthor << " " << author << std::endl;
        }
        if (!manifest.Moniker.empty())
        {
            info << Execution::LabelEmphasis << Resource::String::ShowLabelMoniker << " " << manifest.Moniker << std::endl;
        }
        auto description = manifest.CurrentLocalization.Get<Manifest::Localization::Description>();
        if (description.empty())
        {
            // Fall back to short description
            description = manifest.CurrentLocalization.Get<Manifest::Localization::ShortDescription>();
        }
        if (!description.empty())
        {
            info << Execution::LabelEmphasis << Resource::String::ShowLabelDescription << " " << description << std::endl;
        }
        auto homepage = manifest.CurrentLocalization.Get<Manifest::Localization::PackageUrl>();
        if (!homepage.empty())
        {
            info << Execution::LabelEmphasis << Resource::String::ShowLabelPackageUrl << " " << homepage << std::endl;
        }
        info << Execution::LabelEmphasis << Resource::String::ShowLabelLicense << " " << manifest.CurrentLocalization.Get<Manifest::Localization::License>() << std::endl;
        auto licenseUrl = manifest.CurrentLocalization.Get<Manifest::Localization::LicenseUrl>();
        if (!licenseUrl.empty())
        {
            info << Execution::LabelEmphasis << Resource::String::ShowLabelLicenseUrl << " " << licenseUrl << std::endl;
        }
        auto agreements = manifest.CurrentLocalization.Get<Manifest::Localization::Agreements>();
        if (!agreements.empty())
        {
            context.Reporter.Info() << Execution::LabelEmphasis << Resource::String::ShowLabelAgreements << std::endl;
            for (const auto& agreement : agreements)
            {
                context.Reporter.Info() << "  " << Execution::LabelEmphasis << agreement.Label + ":"  << " " << agreement.TextOrUrl << std::endl;
            }
        }
    }

    void ShowInstallerInfo(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();
        auto info = context.Reporter.Info();

        info << Execution::LabelEmphasis << Resource::String::ShowLabelInstaller << std::endl;
        if (installer)
        {
            info << "  " << Execution::LabelEmphasis << Resource::String::ShowLabelInstallerType << " " << Manifest::InstallerTypeToString(installer->InstallerType) << std::endl;
            if (!installer->Locale.empty())
            {
                info << "  " << Execution::LabelEmphasis << Resource::String::ShowLabelInstallerLocale << " " << installer->Locale << std::endl;
            }
            if (!installer->Url.empty())
            {
                info << "  " << Execution::LabelEmphasis << Resource::String::ShowLabelInstallerUrl << " " << installer->Url << std::endl;
            }
            if (!installer->Sha256.empty())
            {
                info << "  " << Execution::LabelEmphasis << Resource::String::ShowLabelInstallerSha256 << " " << Utility::SHA256::ConvertToString(installer->Sha256) << std::endl;
            }
            if (!installer->ProductId.empty())
            {
                info << "  " << Execution::LabelEmphasis << Resource::String::ShowLabelInstallerProductId << " " << installer->ProductId << std::endl;
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