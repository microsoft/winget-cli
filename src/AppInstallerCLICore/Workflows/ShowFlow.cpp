// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ShowFlow.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        void OutputVersionAndChannel(Execution::Context& context, std::string_view version, std::string_view channel)
        {
            auto out = context.Reporter.Info();

            out << "  " << version;
            if (!channel.empty())
            {
                out << '[' << channel << ']';
            }
            out << std::endl;
        }
    }

    void ShowManifestInfo(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        const auto& installer = context.Get<Execution::Data::Installer>();

        ManifestComparator manifestComparator(context.Args);
        auto selectedLocalization = manifestComparator.GetPreferredLocalization(manifest);

        // TODO: Come up with a prettier format
        context.Reporter.Info() << "Id: " + manifest.Id << std::endl;
        context.Reporter.Info() << "Name: " + manifest.Name << std::endl;
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

        context.Reporter.Info() << manifest.Id << ", " << manifest.Name << std::endl;
        OutputVersionAndChannel(context, manifest.Version, manifest.Channel);
    }

    void ShowAppVersions(Execution::Context& context)
    {
        auto app = context.Get<Execution::Data::SearchResult>().Matches.at(0).Application.get();

        context.Reporter.Info() << app->GetId() << ", " << app->GetName() << std::endl;

        for (auto& version : app->GetVersions())
        {
            OutputVersionAndChannel(context, version.GetVersion().ToString(), version.GetChannel().ToString());
        }
    }
}