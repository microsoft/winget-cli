// -----------------------------------------------------------------------------
// <copyright file="WinGetPackageManagerCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.Properties;

    /// <summary>
    /// Used by Repair-WinGetPackageManager and Assert-WinGetPackageManager.
    /// </summary>
    public sealed class WinGetPackageManagerCommand : BaseCommand
    {
        private const string EnvPath = "env:PATH";

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
            this.RepairStateMachine(expectedVersion, new HashSet<IntegrityCategory>());
        }

        private void RepairStateMachine(string expectedVersion, HashSet<IntegrityCategory> seenCategories)
        {
            try
            {
                WinGetIntegrity.AssertWinGet(this.PsCmdlet, expectedVersion);
                this.PsCmdlet.WriteDebug($"WinGet is in a good state.");
            }
            catch (WinGetIntegrityException e)
            {
                if (seenCategories.Contains(e.Category))
                {
                    this.PsCmdlet.WriteDebug($"{e.Category} encountered previously");
                    throw;
                }

                this.PsCmdlet.WriteDebug($"Integrity category type: {e.Category}");
                seenCategories.Add(e.Category);

                switch (e.Category)
                {
                    case IntegrityCategory.UnexpectedVersion:
                        this.InstallDifferentVersion(new WinGetVersion(expectedVersion));
                        break;
                    case IntegrityCategory.NotInPath:
                        this.RepairEnvPath();
                        break;
                    case IntegrityCategory.AppInstallerNotRegistered:
                        var appxModule = new AppxModuleHelper(this.PsCmdlet);
                        appxModule.RegisterAppInstaller();
                        break;
                    case IntegrityCategory.AppInstallerNotInstalled:
                    case IntegrityCategory.AppInstallerNotSupported:
                    case IntegrityCategory.Failure:
                        // If we are here and expectedVersion is empty, it means that they just ran Repair-WinGetPackageManager.
                        // When there is not version specified, we don't want to assume an empty version means latest, but in
                        // this particular case we need to.
                        if (string.IsNullOrEmpty(expectedVersion))
                        {
                            var gitHubRelease = new GitHubRelease();
                            expectedVersion = gitHubRelease.GetLatestVersionTagName(false);
                        }

                        this.DownloadAndInstall(expectedVersion, false);
                        break;
                    case IntegrityCategory.AppExecutionAliasDisabled:
                        // Sorry, but the user has to manually enabled it.
                        this.PsCmdlet.WriteInformation(Resources.AppExecutionAliasDisabledHelpMessage, WriteInformationTags);
                        throw;
                    case IntegrityCategory.Unknown:
                        throw;
                    default:
                        throw new NotSupportedException();
                }

                this.RepairStateMachine(expectedVersion, seenCategories);
            }
        }

        private void InstallDifferentVersion(WinGetVersion toInstallVersion)
        {
            var installedVersion = WinGetVersion.InstalledWinGetVersion;

            this.PsCmdlet.WriteDebug($"Installed WinGet version {installedVersion.TagVersion}");
            this.PsCmdlet.WriteDebug($"Installing WinGet version {toInstallVersion.TagVersion}");

            bool downgrade = false;
            if (installedVersion.CompareAsDeployment(toInstallVersion) > 0)
            {
                downgrade = true;
            }

            this.DownloadAndInstall(toInstallVersion.TagVersion, downgrade);
        }

        private void DownloadAndInstall(string versionTag, bool downgrade)
        {
            using var tempFile = new TempFile();

            // Download and install.
            var gitHubRelease = new GitHubRelease();
            gitHubRelease.DownloadRelease(versionTag, tempFile.FullPath);

            var appxModule = new AppxModuleHelper(this.PsCmdlet);
            appxModule.AddAppInstallerBundle(tempFile.FullPath, downgrade);
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
