// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace std::string_view_literals;

    // Token specified in installer args will be replaced by proper value.
    static constexpr std::string_view ARG_TOKEN_LOGPATH = "<LOGPATH>"sv;
    static constexpr std::string_view ARG_TOKEN_INSTALLPATH = "<INSTALLPATH>"sv;

    // Ensures that there is an applicable installer.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureApplicableInstaller(Execution::Context& context);

    // Shows the installation disclaimer.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void ShowInstallationDisclaimer(Execution::Context& context);

    // Shows the license agreements if the application has them.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    struct ShowPackageAgreements : public WorkflowTask
    {
        ShowPackageAgreements(bool ensureAcceptance) : WorkflowTask("ShowPackageAgreements"), m_ensureAcceptance(ensureAcceptance) {}

        void operator()(Execution::Context& context) const override;

    private:
        // Whether we need to ensure that the agreements are accepted, or only show them.
        bool m_ensureAcceptance;
    };

    // Ensure the user accepted the license agreements.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    struct EnsurePackageAgreementsAcceptance : public WorkflowTask
    {
        EnsurePackageAgreementsAcceptance(bool showPrompt) : WorkflowTask("EnsurePackageAgreementsAcceptance"), m_showPrompt(showPrompt) {}

        void operator()(Execution::Context& context) const override;

    private:
        // Whether to show an interactive prompt
        bool m_showPrompt;
    };

    // Ensure that the user accepted all the license agreements when there are
    // multiple installers.
    // Required Args: None
    // Inputs: PackagesToInstall
    // Outputs: None
    void EnsurePackageAgreementsAcceptanceForMultipleInstallers(Execution::Context& context);

    // Composite flow that chooses what to do based on the installer type.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void ExecuteInstaller(Execution::Context& context);

    // Runs the installer via ShellExecute.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void ShellExecuteInstall(Execution::Context& context);

    // Runs an MSI installer directly via MSI APIs.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void DirectMSIInstall(Execution::Context& context);

    // Deploys the MSIX.
    // Required Args: None
    // Inputs: Manifest?, Installer || InstallerPath
    // Outputs: None
    void MsixInstall(Execution::Context& context);

    // Reports the return code returned by the installer.
    // Required Args: None
    // Inputs: Manifest, Installer, InstallerResult
    // Outputs: None
    struct ReportInstallerResult : public WorkflowTask
    {
        ReportInstallerResult(std::string_view installerType, HRESULT hr, bool isHResult = false) :
            WorkflowTask("ReportInstallerResult"),
            m_installerType(installerType),
            m_hr(hr),
            m_isHResult(isHResult) {}

        void operator()(Execution::Context& context) const override;

    private:
        // Installer type used when reporting failures.
        std::string_view m_installerType;
        // Result to return if the installer failed.
        HRESULT m_hr;
        // Whether the installer result is an HRESULT. This guides how we show it.
        bool m_isHResult;
    };

    // Reports manifest identity and shows installation disclaimer
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ReportIdentityAndInstallationDisclaimer(Execution::Context& context);

    // Installs a specific package installer. See also InstallSinglePackage & InstallMultiplePackages.
    // Required Args: None
    // Inputs: InstallerPath, Manifest, Installer, PackageVersion, InstalledPackageVersion?
    // Outputs: None
    void InstallPackageInstaller(Execution::Context& context);

    // Downloads the installer for a single package. This also does all the reporting and user interaction needed.
    // Required Args: None
    // Inputs: Manifest, Installer
    // Outputs: InstallerPath
    void DownloadSinglePackage(Execution::Context& context);

    // Installs a single package. This also does the reporting, user interaction, and installer download
    // for single-package installation.
    // RequiredArgs: None
    // Inputs: Manifest, Installer, PackageVersion, InstalledPackageVersion?
    // Outputs: None
    void InstallSinglePackage(Execution::Context& context);

    // Installs multiple packages. This also does the reporting and user interaction needed.
    // Required Args: None
    // Inputs: PackagesToInstall
    // Outputs: None
    struct InstallMultiplePackages : public WorkflowTask
    {
        InstallMultiplePackages(
            StringResource::StringId dependenciesReportMessage,
            HRESULT resultOnFailure,
            std::vector<HRESULT>&& ignorableInstallResults = {},
            bool ensurePackageAgreements = true,
            bool ignoreDependencies = false) :
            WorkflowTask("InstallMultiplePackages"),
            m_dependenciesReportMessage(dependenciesReportMessage),
            m_resultOnFailure(resultOnFailure),
            m_ignorableInstallResults(std::move(ignorableInstallResults)),
            m_ignorePackageDependencies(ignoreDependencies),
            m_ensurePackageAgreements(ensurePackageAgreements) {}

        void operator()(Execution::Context& context) const override;

    private:
        HRESULT m_resultOnFailure;
        std::vector<HRESULT> m_ignorableInstallResults;
        StringResource::StringId m_dependenciesReportMessage;
        bool m_ignorePackageDependencies;
        bool m_ensurePackageAgreements;
    };

    // Stores the existing set of packages in ARP.
    // Required Args: None
    // Inputs: Installer
    // Outputs: ARPSnapshot
    void SnapshotARPEntries(Execution::Context& context);

    // Reports on the changes between the stored ARPSnapshot and the current values,
    // and stores the product code of the ARP entry found for the package.
    // Required Args: None
    // Inputs: ARPSnapshot?, Manifest, PackageVersion
    // Outputs: CorrelatedAppsAndFeaturesEntries?
    void ReportARPChanges(Execution::Context& context);

    // Records the installation to the tracking catalog.
    // Required Args: None
    // Inputs: PackageVersion?, Manifest, Installer, CorrelatedAppsAndFeaturesEntries?
    // Outputs: None
    void RecordInstall(Execution::Context& context);
}
