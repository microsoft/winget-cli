﻿// -----------------------------------------------------------------------------
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
    [OutputType(typeof(int))]
    public class RepairCommand : BaseCommand
    {
        private const string EnvPath = "env:PATH";
        private const int Succeeded = 0;
        private const int Failed = -1;

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
            int result = Failed;

            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this, this.Version);

            this.WriteDebug($"Integrity category type: {integrityCategory}");

            bool preRelease = this.IncludePreRelease.ToBool();

            if (integrityCategory == IntegrityCategory.Installed ||
                integrityCategory == IntegrityCategory.UnexpectedVersion)
            {
                result = this.RepairForInstalled(preRelease, WinGetVersionHelper.InstalledWinGetVersion, this.Version);
            }
            else if (integrityCategory == IntegrityCategory.NotInPath)
            {
                this.RepairEnvPath();
                result = Succeeded;
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotRegistered)
            {
                var appxModule = new AppxModuleHelper(this);
                appxModule.RegisterAppInstaller();

                this.WriteDebug($"WinGet version {WinGetVersionHelper.InstalledWinGetVersion} registered");
                result = Succeeded;
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotInstalled ||
                     integrityCategory == IntegrityCategory.AppInstallerNotSupported ||
                     integrityCategory == IntegrityCategory.Failure)
            {
                if (this.DownloadAndInstall(preRelease, this.Version, false))
                {
                    result = Succeeded;
                }
            }
            else if (integrityCategory == IntegrityCategory.AppExecutionAliasDisabled)
            {
                // Sorry, but the user has to manually enabled it.
                this.WriteInformation(Resources.AppExecutionAliasDisabledHelpMessage, WriteInformationTags);
            }
            else
            {
                this.WriteInformation(Resources.WinGetNotSupportedMessage, WriteInformationTags);
            }

            this.WriteObject(result);
        }

        private int RepairForInstalled(bool preRelease, string installedVersion, string toInstallVersion)
        {
            if (string.IsNullOrEmpty(toInstallVersion))
            {
                var gitHubRelease = new GitHubRelease();
                toInstallVersion = gitHubRelease.GetLatestVersionTagName(preRelease);
            }

            if (toInstallVersion != installedVersion)
            {
                this.WriteDebug($"Installed WinGet version {installedVersion}");
                this.WriteDebug($"Installing WinGet version {toInstallVersion}");

                var v1 = WinGetVersionHelper.ConvertWinGetVersion(installedVersion);
                var v2 = WinGetVersionHelper.ConvertWinGetVersion(toInstallVersion);

                bool downgrade = false;
                if (v1.CompareTo(v2) > 0)
                {
                    downgrade = true;
                }

                if (this.DownloadAndInstall(preRelease, toInstallVersion, downgrade))
                {
                    return Succeeded;
                }
            }
            else
            {
                this.WriteDebug($"Installed WinGet version and target match {WinGetVersionHelper.InstalledWinGetVersion}");
                return Succeeded;
            }

            return Failed;
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
            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this, versionTag);
            if (integrityCategory != IntegrityCategory.Installed)
            {
                return false;
            }

            this.WriteDebug($"Installed WinGet version {versionTag}");
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
