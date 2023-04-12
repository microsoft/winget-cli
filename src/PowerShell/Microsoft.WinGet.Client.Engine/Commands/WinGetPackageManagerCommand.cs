// -----------------------------------------------------------------------------
// <copyright file="WinGetPackageManagerCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.Properties;

    /// <summary>
    /// Used by Rapair-WinGetPackageManager and Asser-WinGetPackageManager.
    /// </summary>
    public sealed class WinGetPackageManagerCommand : BaseCommand
    {
        private const string EnvPath = "env:PATH";
        private const int Succeeded = 0;
        private const int Failed = -1;

        private static readonly string[] WriteInformationTags = new string[] { "PSHOST" };

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetPackageManagerCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Cmdlet being executed.</param>
        public WinGetPackageManagerCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Asserts winget version is the latest version on winget-cli.
        /// </summary>
        /// <param name="preRelease">Use prerelease version on GitHub.</param>
        public void AssertUsingLatest(bool preRelease)
        {
            var gitHubRelease = new GitHubRelease();
            string expectedVersion = gitHubRelease.GetLatestVersionTagName(preRelease);
            this.Assert(expectedVersion);
        }

        /// <summary>
        /// Asserts the version installed is the specified.
        /// </summary>
        /// <param name="expectedVersion">The expected version.</param>
        public void Assert(string expectedVersion)
        {
            WinGetIntegrity.AssertWinGet(this.PsCmdlet, expectedVersion);
        }

        /// <summary>
        /// Repairs winget using the latest version on winget-cli.
        /// </summary>
        /// <param name="preRelease">Use prerelease version on GitHub.</param>
        public void RepairUsingLatest(bool preRelease)
        {
            var gitHubRelease = new GitHubRelease();
            string expectedVersion = gitHubRelease.GetLatestVersionTagName(preRelease);
            this.Repair(expectedVersion);
        }

        /// <summary>
        /// Repairs winget if needed.
        /// </summary>
        /// <param name="expectedVersion">The expected version, if any.</param>
        public void Repair(string expectedVersion)
        {
            int result = Failed;

            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this.PsCmdlet, expectedVersion);
            this.PsCmdlet.WriteDebug($"Integrity category type: {integrityCategory}");

            if (integrityCategory == IntegrityCategory.Installed ||
                integrityCategory == IntegrityCategory.UnexpectedVersion)
            {
                result = this.VerifyWinGetInstall(integrityCategory, expectedVersion);
            }
            else if (integrityCategory == IntegrityCategory.NotInPath)
            {
                this.RepairEnvPath();

                // Now try again and get the desired winget version if needed.
                var newIntegrityCategory = WinGetIntegrity.GetIntegrityCategory(this.PsCmdlet, expectedVersion);
                this.PsCmdlet.WriteDebug($"Integrity category after fixing PATH {newIntegrityCategory}");
                result = this.VerifyWinGetInstall(newIntegrityCategory, expectedVersion);
            }
            else if (integrityCategory == IntegrityCategory.AppInstallerNotRegistered)
            {
                var appxModule = new AppxModuleHelper(this.PsCmdlet);
                appxModule.RegisterAppInstaller();

                // Now try again and get the desired winget version if needed.
                var newIntegrityCategory = WinGetIntegrity.GetIntegrityCategory(this.PsCmdlet, expectedVersion);
                this.PsCmdlet.WriteDebug($"Integrity category after registering {newIntegrityCategory}");
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
                    this.PsCmdlet.WriteDebug($"Failed installing {expectedVersion}");
                }
            }
            else if (integrityCategory == IntegrityCategory.AppExecutionAliasDisabled)
            {
                // Sorry, but the user has to manually enabled it.
                this.PsCmdlet.WriteInformation(Resources.AppExecutionAliasDisabledHelpMessage, WriteInformationTags);
            }
            else
            {
                this.PsCmdlet.WriteInformation(Resources.WinGetNotSupportedMessage, WriteInformationTags);
            }

            this.PsCmdlet.WriteObject(result);
        }

        private int VerifyWinGetInstall(IntegrityCategory integrityCategory, string expectedVersion)
        {
            if (integrityCategory == IntegrityCategory.Installed)
            {
                // Nothing to do
                this.PsCmdlet.WriteDebug($"WinGet is in a good state.");
                return Succeeded;
            }
            else if (integrityCategory == IntegrityCategory.UnexpectedVersion)
            {
                // The versions are different, download and install.
                if (!this.InstallDifferentVersion(new WinGetVersion(expectedVersion)))
                {
                    this.PsCmdlet.WriteDebug($"Failed installing {expectedVersion}");
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

            this.PsCmdlet.WriteDebug($"Installed WinGet version {installedVersion.TagVersion}");
            this.PsCmdlet.WriteDebug($"Installing WinGet version {toInstallVersion.TagVersion}");

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

            var appxModule = new AppxModuleHelper(this.PsCmdlet);
            appxModule.AddAppInstallerBundle(downloadedMsixBundlePath, downgrade);

            // Verify that is installed
            var integrityCategory = WinGetIntegrity.GetIntegrityCategory(this.PsCmdlet, versionTag);
            if (integrityCategory != IntegrityCategory.Installed)
            {
                return false;
            }

            this.PsCmdlet.WriteDebug($"Installed WinGet version {versionTag}");
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
            this.PsCmdlet.SessionState.PSVariable.Set(EnvPath, newPwshPathEnv);

            this.PsCmdlet.WriteDebug($"PATH environment variable updated");
        }
    }
}
