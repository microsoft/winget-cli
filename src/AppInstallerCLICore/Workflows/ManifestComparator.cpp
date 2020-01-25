// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Common.h"
#include "WorkflowReporter.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Manifest;

namespace AppInstaller::Workflow
{
    bool InstallerComparator::operator() (const ManifestInstaller& installer1, const ManifestInstaller& installer2)
    {
        // Todo: Comapre only architecture for now. Need more work and spec.
        if (Utility::IsApplicableArchitecture(installer1.Arch) < Utility::IsApplicableArchitecture(installer2.Arch))
        {
            return false;
        }

        return true;
    }

    bool LocalizationComparator::operator() (const ManifestLocalization& loc1, const ManifestLocalization& loc2)
    {
        UNREFERENCED_PARAMETER(loc2);

        // Todo: Compare simple language for now. Need more work and spec.
        std::string userPreferredLocale = std::locale("").name();

        auto found = userPreferredLocale.find(loc1.Language);

        if (found != std::string::npos)
        {
            return false;
        }

        return true;
    }

    ManifestInstaller ManifestComparator::GetPreferredInstaller(const std::locale& preferredLocale)
    {
        AICLI_LOG(CLI, Info, << "Starting installer selection. Preferred locale: " << preferredLocale.name());

        // Sorting the list of availlable installers according to rules defined in InstallerComparator.
        std::sort(m_manifestRef.Installers.begin(), m_manifestRef.Installers.end(), InstallerComparator());

        // If the first one is inapplicable, then no installer is applicable.
        if (Utility::IsApplicableArchitecture(m_manifestRef.Installers[0].Arch) == -1)
        {
            m_reporterRef.ShowMsg(WorkflowReporter::Level::Error, "No applicable installer found.");
            THROW_EXCEPTION_MSG(WorkflowException(APPINSTALLER_CLI_ERROR_WORKFLOW_FAILED), "No installer with applicable architecture found.");
        }

        ManifestInstaller selectedInstaller = m_manifestRef.Installers[0];

        AICLI_LOG(CLI, Info, << "Completed installer selection.");
        AICLI_LOG(CLI, Verbose, << "Selected installer arch: " << (int)selectedInstaller.Arch);
        AICLI_LOG(CLI, Verbose, << "Selected installer url: " << selectedInstaller.Url);
        AICLI_LOG(CLI, Verbose, << "Selected installer InstallerType: " << selectedInstaller.InstallerType);
        AICLI_LOG(CLI, Verbose, << "Selected installer scope: " << selectedInstaller.Scope);
        AICLI_LOG(CLI, Verbose, << "Selected installer language: " << selectedInstaller.Language);

        return selectedInstaller;
    }

    ManifestLocalization ManifestComparator::GetPreferredLocalization(const std::locale& preferredLocale)
    {
        AICLI_LOG(CLI, Info, << "Starting localization selection. Preferred locale: " << preferredLocale.name());

        ManifestLocalization selectedLocalization;

        // Sorting the list of availlable localizations according to rules defined in LocalizationComparator.
        if (!m_manifestRef.Localization.empty())
        {
            std::sort(m_manifestRef.Localization.begin(), m_manifestRef.Localization.end(), LocalizationComparator());

            // TODO: needs to check language applicability here

            selectedLocalization = m_manifestRef.Localization[0];
        }
        else
        {
            // Pupulate default from package manifest
            selectedLocalization.Description = m_manifestRef.Description;
            selectedLocalization.Homepage = m_manifestRef.Homepage;
            selectedLocalization.LicenseUrl = m_manifestRef.LicenseUrl;
        }

        AICLI_LOG(CLI, Info, << "Completed localization selection. Selected localization language: " << selectedLocalization.Language);

        return selectedLocalization;
    }
}