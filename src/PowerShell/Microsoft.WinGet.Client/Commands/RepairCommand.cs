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

    /// <summary>
    /// Repair-WinGet. Repairs winget if needed.
    /// </summary>
    [Cmdlet(VerbsDiagnostic.Repair, Constants.WinGetNouns.WinGet)]
    public class RepairCommand : BaseCommand
    {
        private const string EnvPath = "env:PATH";

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
            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this.InvokeCommand);

            bool preRelease = this.IncludePreRelease.ToBool();

            if (integrityCategory == IntegrityCategory.Installed)
            {
                string toInstallVersion = this.Version;
                var gitHubRelease = new GitHubRelease();

                if (string.IsNullOrEmpty(toInstallVersion))
                {
                    toInstallVersion = gitHubRelease.GetLatestVersionTagName(preRelease);
                }

                if (toInstallVersion != WinGetVersionHelper.InstalledWinGetVersion)
                {
                    this.WriteObject($"Current installed version {WinGetVersionHelper.InstalledWinGetVersion}");
                    this.WriteObject($"Version to install {toInstallVersion}");

                    var downloadedMsixBundlePath = gitHubRelease.DownloadRelease(
                        preRelease,
                        toInstallVersion);

                    var installedVersion = WinGetVersionHelper.ConvertInstalledWinGetVersion();
                    var inputVersion = WinGetVersionHelper.ConvertWinGetVersion(toInstallVersion);

                    bool downgrade = false;
                    if (installedVersion.CompareTo(inputVersion) > 0)
                    {
                        downgrade = true;
                    }

                    var appxModule = new AppxModuleHelper(this.InvokeCommand);
                    appxModule.AddAppInstallerBundle(downloadedMsixBundlePath, downgrade);
                }
                else
                {
                    this.WriteObject("WinGet is in good state");
                }
            }
            else if (integrityCategory == IntegrityCategory.NotInPath)
            {
                // Add windows app path to user PATH environment variable
                Utilities.AddWindowsAppToPath();

                // Update this sessions PowerShell environment so the user doesn't have to restart the terminal.
                string envPathUser = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.User);
                string envPathMachine = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.Machine);
                string newPwshPathEnv = $"{envPathMachine};{envPathUser}";
                this.SessionState.PSVariable.Set(EnvPath, newPwshPathEnv);
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotRegistered)
            {
                var appxModule = new AppxModuleHelper(this.InvokeCommand);
                appxModule.RegisterAppInstaller();
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotInstalled ||
                     integrityCategory == IntegrityCategory.AppInstallerNotSupported ||
                     integrityCategory == IntegrityCategory.Failure)
            {
                // Download and install.
                var gitHubRelease = new GitHubRelease();
                var downloadedMsixBundlePath = gitHubRelease.DownloadRelease(
                    this.IncludePreRelease.ToBool(),
                    this.Version);

                var appxModule = new AppxModuleHelper(this.InvokeCommand);
                appxModule.AddAppInstallerBundle(downloadedMsixBundlePath);

                this.WriteObject($"Version to install {WinGetVersionHelper.InstalledWinGetVersion}");
            }
            else if (integrityCategory == IntegrityCategory.AppExecutionAliasDisabled)
            {
                // Sorry, but the user has to manually enabled it.
                throw new Exception("app installer");
            }
            else
            {
                // Unknown
                // OsNotSupported
                throw new Exception("impossible");
            }
        }
    }
}
