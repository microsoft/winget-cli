// -----------------------------------------------------------------------------
// <copyright file="RepairWinGetPackageManagerCommand.cs" company="Microsoft Corporation">
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
    /// Repair-WinGetPackageManager. Repairs winget if needed.
    /// </summary>
    [Cmdlet(
        VerbsDiagnostic.Repair,
        Constants.WinGetNouns.WinGetPackageManager,
        DefaultParameterSetName = Constants.IntegrityVersionSet)]
    [OutputType(typeof(int))]
    public class RepairWinGetPackageManagerCommand : BaseIntegrityCommand
    {
        private const string EnvPath = "env:PATH";
        private const int Succeeded = 0;
        private const int Failed = -1;

        private static readonly string[] WriteInformationTags = new string[] { "PSHOST" };

        /// <summary>
        /// Attempts to repair winget.
        /// TODO: consider WhatIf and Confirm options.
        /// </summary>
        protected override void ProcessRecord()
        {
            int result = Failed;

            string expectedVersion = this.Version;
            if (this.ParameterSetName == Constants.IntegrityLatestSet)
            {
                var gitHubRelease = new GitHubRelease();
                expectedVersion = gitHubRelease.GetLatestVersionTagName(this.IncludePreRelease.ToBool());
            }

            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this, expectedVersion);
            this.WriteDebug($"Integrity category type: {integrityCategory}");

            if (integrityCategory == IntegrityCategory.Installed ||
                integrityCategory == IntegrityCategory.UnexpectedVersion)
            {
                result = this.VerifyWinGetInstall(integrityCategory, expectedVersion);
            }
            else if (integrityCategory == IntegrityCategory.NotInPath)
            {
                this.RepairEnvPath();

                // Now try again and get the desired winget version if needed.
                var newIntegrityCategory = WinGetIntegrity.GetIntegrityCategory(this, expectedVersion);
                this.WriteDebug($"Integrity category after fixing PATH {newIntegrityCategory}");
                result = this.VerifyWinGetInstall(newIntegrityCategory, expectedVersion);
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotRegistered)
            {
                var appxModule = new AppxModuleHelper(this);
                appxModule.RegisterAppInstaller();

                // Now try again and get the desired winget version if needed.
                var newIntegrityCategory = WinGetIntegrity.GetIntegrityCategory(this, expectedVersion);
                this.WriteDebug($"Integrity category after registering {newIntegrityCategory}");
                result = this.VerifyWinGetInstall(newIntegrityCategory, expectedVersion);
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotInstalled ||
                     integrityCategory == IntegrityCategory.AppInstallerNotSupported ||
                     integrityCategory == IntegrityCategory.Failure)
            {
                // If we are here and expectedVersion is empty, it means that they just ran Repair-WinGetPackageManager.
                // When there is not version specified, we don't want to assume an empty version means latest, but in
                // this particular case we need to.
                if (string.IsNullOrEmpty(expectedVersion))
                {
                    var gitHubRelease = new GitHubRelease();
                    expectedVersion = gitHubRelease.GetLatestVersionTagName(false);
                }

                if (this.DownloadAndInstall(expectedVersion, false))
                {
                    result = Succeeded;
                }
                else
                {
                    this.WriteDebug($"Failed installing {expectedVersion}");
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

        private int VerifyWinGetInstall(IntegrityCategory integrityCategory, string expectedVersion)
        {
            if (integrityCategory == IntegrityCategory.Installed)
            {
                // Nothing to do
                this.WriteDebug($"WinGet is in a good state.");
                return Succeeded;
            }
            else if (integrityCategory == IntegrityCategory.UnexpectedVersion)
            {
                // The versions are different, download and install.
                if (!this.InstallDifferentVersion(new WinGetVersion(expectedVersion)))
                {
                    this.WriteDebug($"Failed installing {expectedVersion}");
                }
                else
                {
                    return Succeeded;
                }
            }

            return Failed;
        }

        private bool InstallDifferentVersion(WinGetVersion toInstallVersion)
        {
            var installedVersion = WinGetVersion.InstalledWinGetVersion;

            this.WriteDebug($"Installed WinGet version {installedVersion.TagVersion}");
            this.WriteDebug($"Installing WinGet version {toInstallVersion.TagVersion}");

            bool downgrade = false;
            if (installedVersion.CompareAsDeployment(toInstallVersion) > 0)
            {
                downgrade = true;
            }

            return this.DownloadAndInstall(toInstallVersion.TagVersion, downgrade);
        }

        private bool DownloadAndInstall(string versionTag, bool downgrade)
        {
            // Download and install.
            var gitHubRelease = new GitHubRelease();
            var downloadedMsixBundlePath = gitHubRelease.DownloadRelease(versionTag);

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
