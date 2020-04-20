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
        context.Reporter.Info() << "Version: " + manifest.Version << std::endl;
        context.Reporter.Info() << "Author: " + manifest.Author << std::endl;
        context.Reporter.Info() << "AppMoniker: " + manifest.AppMoniker << std::endl;
        context.Reporter.Info() << "Description: " + selectedLocalization.Description << std::endl;
        context.Reporter.Info() << "Homepage: " + selectedLocalization.Homepage << std::endl;
        context.Reporter.Info() << "License: " + selectedLocalization.LicenseUrl << std::endl;

        context.Reporter.Info() << "Installer:" << std::endl;
        if (installer)
        {
            context.Reporter.Info() << "  Language: " + installer->Language << std::endl;
            context.Reporter.Info() << "  SHA256: " + Utility::SHA256::ConvertToString(installer->Sha256) << std::endl;
            context.Reporter.Info() << "  Download Url: " + installer->Url << std::endl;
            context.Reporter.Info() << "  Type: " + Manifest::ManifestInstaller::InstallerTypeToString(installer->InstallerType) << std::endl;
        }
        else
        {
            context.Reporter.Warn() << "  No installers are applicable to the current system" << std::endl;
        }
    }

    void ShowManifestVersion(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();

        Execution::TableOutput<2> table(context.Reporter, { "Version", "Channel" });
        table.OutputLine({ manifest.Version, manifest.Channel });
        table.Complete();
    }

    void ShowAppVersions(Execution::Context& context)
    {
        auto app = context.Get<Execution::Data::SearchResult>().Matches.at(0).Application.get();

        Execution::TableOutput<2> table(context.Reporter, { "Version", "Channel" });
        for (auto& version : app->GetVersions())
        {
            table.OutputLine({ version.GetVersion().ToString(), version.GetChannel().ToString() });
        }
        table.Complete();
    }
}