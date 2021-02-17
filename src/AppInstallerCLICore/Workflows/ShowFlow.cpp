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

        ManifestComparator manifestComparator(context.Args);
        auto selectedLocalization = manifestComparator.GetPreferredLocalization(manifest);

        // TODO: Come up with a prettier format
        context.Reporter.Info() << "Version: " << manifest.Version << std::endl;
        context.Reporter.Info() << "Publisher: " << manifest.Publisher << std::endl;
        if (!manifest.Author.empty())
        {
            context.Reporter.Info() << "Author: " << manifest.Author << std::endl;
        }
        if (!manifest.AppMoniker.empty())
        {
            context.Reporter.Info() << "AppMoniker: " << manifest.AppMoniker << std::endl;
        }
        if (!selectedLocalization.Description.empty())
        {
            context.Reporter.Info() << "Description: " << selectedLocalization.Description << std::endl;
        }
        if (!selectedLocalization.Homepage.empty())
        {
            context.Reporter.Info() << "Homepage: " << selectedLocalization.Homepage << std::endl;
        }
        if (!manifest.License.empty())
        {
            context.Reporter.Info() << "License: " << manifest.License << std::endl;
        }
        if (!selectedLocalization.LicenseUrl.empty())
        {
            context.Reporter.Info() << "License Url: " << selectedLocalization.LicenseUrl << std::endl;
        }

        context.Reporter.Info() << "Installer:" << std::endl;
        if (installer)
        {
            context.Reporter.Info() << "  Type: " << Manifest::ManifestInstaller::InstallerTypeToString(installer->InstallerType) << std::endl;
            if (!installer->Language.empty())
            {
                context.Reporter.Info() << "  Language: " << installer->Language << std::endl;
            }
            if (!installer->Url.empty())
            {
                context.Reporter.Info() << "  Download Url: " << installer->Url << std::endl;
            }
            if (!installer->Sha256.empty())
            {
                context.Reporter.Info() << "  SHA256: " << Utility::SHA256::ConvertToString(installer->Sha256) << std::endl;
            }
            if (!installer->ProductId.empty())
            {
                context.Reporter.Info() << "  Store Product Id: " << installer->ProductId << std::endl;
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