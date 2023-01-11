// -----------------------------------------------------------------------------
// <copyright file="RepairCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Helpers;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Repair-WinGet. Repairs winget if needed.
    /// </summary>
    [Cmdlet(VerbsDiagnostic.Repair, Constants.WinGetNouns.WinGet)]
    public class RepairCommand : BaseCommand
    {
        private const string EnvPath = "env:PATH";
        private static readonly string[] WriteInformationTags = new string[] { "PSHOST" };

        /// <summary>
        /// Gets or sets the optional version.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Version { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets a value indicating whether to include prerelease winget versions.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter IncludePreRelease { get; set; }

        /// <summary>
        /// Attempts to repair winget.
        /// TODO: consider WhatIf and Confirm options.
        /// </summary>
        protected override void ProcessRecord()
        {
            RepairResult repairResult = RepairResult.Failure;

            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this);

            this.WriteDebug($"Integrity category type: {integrityCategory}");

            bool preRelease = this.IncludePreRelease.ToBool();

            if (integrityCategory == IntegrityCategory.Installed)
            {
                repairResult = this.RepairForInstalled(preRelease, this.Version);
            }
            else if (integrityCategory == IntegrityCategory.NotInPath)
            {
                this.RepairEnvPath();
                repairResult = RepairResult.PathUpdated;
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotRegistered)
            {
                var appxModule = new AppxModuleHelper(this);
                appxModule.RegisterAppInstaller();

                this.WriteDebug($"WinGet version {WinGetVersionHelper.InstalledWinGetVersion} registered");
                repairResult = RepairResult.Registered;
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotInstalled ||
                     integrityCategory == IntegrityCategory.AppInstallerNotSupported ||
                     integrityCategory == IntegrityCategory.Failure)
            {
                if (this.DownloadAndInstall(preRelease, this.Version, false))
                {
                    repairResult = RepairResult.Installed;
                }
            }
            else if (integrityCategory == IntegrityCategory.AppExecutionAliasDisabled)
            {
                // Sorry, but the user has to manually enabled it.
                this.WriteInformation(Resources.AppExecutionAliasDisabledHelpMessage, WriteInformationTags);
                repairResult = RepairResult.NeedsManualRepair;
            }
            else
            {
                this.WriteInformation(Resources.WinGetNotSupportedMessage, WriteInformationTags);
            }

            this.WriteObject(repairResult);
        }

        private RepairResult RepairForInstalled(bool preRelease, string toInstallVersion)
        {
            RepairResult repairResult = RepairResult.Failure;

            if (string.IsNullOrEmpty(toInstallVersion))
            {
                var gitHubRelease = new GitHubRelease();
                toInstallVersion = gitHubRelease.GetLatestVersionTagName(preRelease);
            }

            if (toInstallVersion != WinGetVersionHelper.InstalledWinGetVersion)
            {
                this.WriteDebug($"Installed WinGet version {WinGetVersionHelper.InstalledWinGetVersion}");
                this.WriteDebug($"Installing WinGet version {toInstallVersion}");

                var installedVersion = WinGetVersionHelper.ConvertInstalledWinGetVersion();
                var inputVersion = WinGetVersionHelper.ConvertWinGetVersion(toInstallVersion);

                bool downgrade = false;
                if (installedVersion.CompareTo(inputVersion) > 0)
                {
                    downgrade = true;
                }

                if (this.DownloadAndInstall(preRelease, toInstallVersion, downgrade))
                {
                    repairResult = downgrade ? RepairResult.Downgraded : RepairResult.Updated;
                }
            }
            else
            {
                this.WriteDebug($"Installed WinGet version and target match {WinGetVersionHelper.InstalledWinGetVersion}");
                repairResult = RepairResult.Noop;
            }

            return repairResult;
        }

        private bool DownloadAndInstall(bool preRelease, string versionTag, bool downgrade)
        {
            // Download and install.
            var gitHubRelease = new GitHubRelease();
            var downloadedMsixBundlePath = gitHubRelease.DownloadRelease(
                preRelease,
                versionTag);

            var appxModule = new AppxModuleHelper(this);
            appxModule.AddAppInstallerBundle(downloadedMsixBundlePath, downgrade);

            // Verify that is installed
            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this);
            if (integrityCategory != IntegrityCategory.Installed)
            {
                return false;
            }

            this.WriteDebug($"Installed WinGet version {WinGetVersionHelper.InstalledWinGetVersion}");
            return true;
        }

        private void RepairEnvPath()
        {
            // Add windows app path to user PATH environment variable
            Utilities.AddWindowsAppToPath();

            // Update this sessions PowerShell environment so the user doesn't have to restart the terminal.
            string envPathUser = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.User);
            string envPathMachine = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.Machine);
            string newPwshPathEnv = $"{envPathMachine};{envPathUser}";
            this.SessionState.PSVariable.Set(EnvPath, newPwshPathEnv);

            this.WriteDebug($"PATH environment variable updated");
        }
    }
}
