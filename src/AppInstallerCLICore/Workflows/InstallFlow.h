// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionContext.h"
#include <winget/Manifest.h>

namespace AppInstaller::CLI::Workflow
{
    using namespace std::string_view_literals;

    // Token specified in installer args will be replaced by proper value.
    static constexpr std::string_view ARG_TOKEN_LOGPATH = "<LOGPATH>"sv;
    static constexpr std::string_view ARG_TOKEN_INSTALLPATH = "<INSTALLPATH>"sv;

    // Determines if an installer type is allowed to install/uninstall in parallel.
    bool ExemptFromSingleInstallLocking(AppInstaller::Manifest::InstallerTypeEnum type);

    namespace details
    {
        // These single type install flows should remain "internal" and only ExecuteInstallerForType should be used externally
        // so that all installs can properly handle single install locking.

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

        // Runs the flow for installing a Portable package.
        // Required Args: None
        // Inputs: Installer, InstallerPath
        // Outputs: None
        void PortableInstall(Execution::Context& context);

        // Runs the flow for installing a package from an archive.
        // Required Args: None
        // Inputs: Installer, InstallerPath, Manifest
        // Outputs: None
        void ArchiveInstall(Execution::Context& context);
    }

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

    // Displays the installations notes after a successful install.
    // Required Args: None
    // Inputs: InstallationNotes
    // Outputs: None
    void DisplayInstallationNotes(Execution::Context& context);
    
    // Checks if there are any included arguments that are not supported for the package.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void CheckForUnsupportedArgs(Execution::Context& context);

    // Admin is required for machine scope install for installer types like portable, msix and msstore.
    // Required Args: None
    // Inputs: Installer
    // Outputs: None
    void EnsureRunningAsAdminForMachineScopeInstall(Execution::Context& context);

    // Composite flow that chooses what to do based on the installer type.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    void ExecuteInstaller(Execution::Context& context);

    // Composite flow that chooses what to do based on the installer type.
    // Required Args: None
    // Inputs: Installer, InstallerPath
    // Outputs: None
    struct ExecuteInstallerForType : public WorkflowTask
    {
        ExecuteInstallerForType(Manifest::InstallerTypeEnum installerType) : WorkflowTask("ExecuteInstallerForType"), m_installerType(installerType) {}

        void operator()(Execution::Context& context) const override;

    private:
        Manifest::InstallerTypeEnum m_installerType;
    };

    // Verifies parameters for install to ensure success.
    // Required Args: None
    // Inputs: 
    // Outputs: None
    void EnsureSupportForInstall(Execution::Context& context);

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

    // Installs a specific package installer. See also InstallSinglePackage & ProcessMultiplePackages
    // Required Args: None
    // Inputs: InstallerPath, Manifest, Installer, PackageVersion, InstalledPackageVersion?
    // Outputs: None
    void InstallPackageInstaller(Execution::Context& context);
    
    // Installs the dependencies for a specific package.
    // Required Args: None
    // Inputs: InstallerPath, Manifest, Installer, PackageVersion, InstalledPackageVersion?
    // Outputs: None
    void InstallDependencies(Execution::Context& context);

    // Downloads all of the package dependencies of a specific package. Only used in the 'winget download' and COM download flows.
    // Required Args: none
    // Inputs: Manifest, Installer
    // Outputs: None
    void DownloadPackageDependencies(Execution::Context& context);

    // Installs a single package. This also does the reporting, user interaction, and installer download
    // for single-package installation.
    // RequiredArgs: None
    // Inputs: Manifest, Installer, PackageVersion, InstalledPackageVersion?
    // Outputs: None
    void InstallSinglePackage(Execution::Context& context);

    // Processes multiple packages by handling download and/or install. This also does the reporting and user interaction needed.
    // Required Args: None
    // Inputs: PackageSubContexts
    // Outputs: None
    struct ProcessMultiplePackages : public WorkflowTask
    {
        ProcessMultiplePackages(
            StringResource::StringId dependenciesReportMessage,
            HRESULT resultOnFailure,
            std::vector<HRESULT>&& ignorableInstallResults = {},
            bool ensurePackageAgreements = true,
            bool ignoreDependencies = false,
            bool stopOnFailure = false,
            bool refreshPathVariable = false):
            WorkflowTask("ProcessMultiplePackages"),
            m_dependenciesReportMessage(dependenciesReportMessage),
            m_resultOnFailure(resultOnFailure),
            m_ignorableInstallResults(std::move(ignorableInstallResults)),
            m_ignorePackageDependencies(ignoreDependencies),
            m_ensurePackageAgreements(ensurePackageAgreements),
            m_stopOnFailure(stopOnFailure),
            m_refreshPathVariable(refreshPathVariable){}

        void operator()(Execution::Context& context) const override;

    private:
        HRESULT m_resultOnFailure;
        std::vector<HRESULT> m_ignorableInstallResults;
        StringResource::StringId m_dependenciesReportMessage;
        bool m_ignorePackageDependencies;
        bool m_ensurePackageAgreements;
        bool m_stopOnFailure;
        bool m_refreshPathVariable;
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
