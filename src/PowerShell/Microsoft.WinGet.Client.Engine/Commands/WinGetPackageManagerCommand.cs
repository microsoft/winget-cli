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
        /// <param name="allUsers">Install for all users. Requires admin.</param>
        public void RepairUsingLatest(bool preRelease, bool allUsers)
        {
            var gitHubRelease = new GitHubRelease();
            string expectedVersion = gitHubRelease.GetLatestVersionTagName(preRelease);
            this.Repair(expectedVersion, allUsers);
        }

        /// <summary>
        /// Repairs winget if needed.
        /// </summary>
        /// <param name="expectedVersion">The expected version, if any.</param>
        /// <param name="allUsers">Install for all users. Requires admin.</param>
        public void Repair(string expectedVersion, bool allUsers)
        {
            this.RepairStateMachine(expectedVersion, allUsers, new HashSet<IntegrityCategory>());
        }

        private void RepairStateMachine(string expectedVersion, bool allUsers, HashSet<IntegrityCategory> seenCategories)
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
                        this.InstallDifferentVersion(new WinGetVersion(expectedVersion), allUsers);
                        break;
                    case IntegrityCategory.NotInPath:
                        this.RepairEnvPath();
                        break;
                    case IntegrityCategory.AppInstallerNotRegistered:
                        this.Register();
                        break;
                    case IntegrityCategory.AppInstallerNotInstalled:
                    case IntegrityCategory.AppInstallerNotSupported:
                    case IntegrityCategory.Failure:
                        this.Install(expectedVersion, allUsers);
                        break;
                    case IntegrityCategory.AppInstallerNoLicense:
                        if (!allUsers)
                        {
                            // This requires -AllUsers.
                            throw;
                        }

                        this.Install(expectedVersion, allUsers);
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

                this.RepairStateMachine(expectedVersion, allUsers, seenCategories);
            }
        }

        private void InstallDifferentVersion(WinGetVersion toInstallVersion, bool allUsers)
        {
            var installedVersion = WinGetVersion.InstalledWinGetVersion;

            this.PsCmdlet.WriteDebug($"Installed WinGet version {installedVersion.TagVersion}");
            this.PsCmdlet.WriteDebug($"Installing WinGet version {toInstallVersion.TagVersion}");

            var appxModule = new AppxModuleHelper(this.PsCmdlet);
            if (installedVersion.CompareAsDeployment(toInstallVersion) > 0)
            {
                appxModule.InstallDowngrade(toInstallVersion.TagVersion, allUsers);
            }
            else
            {
                appxModule.Install(toInstallVersion.TagVersion, allUsers);
            }
        }

        private void Install(string expectedVersion, bool allUsers)
        {
            // If we are here and expectedVersion is empty, it means that they just ran Repair-WinGetPackageManager.
            // When there is not version specified, we don't want to assume an empty version means latest, but in
            // this particular case we need to.
            if (string.IsNullOrEmpty(expectedVersion))
            {
                var gitHubRelease = new GitHubRelease();
                expectedVersion = gitHubRelease.GetLatestVersionTagName(false);
            }

            var appxModule = new AppxModuleHelper(this.PsCmdlet);
            appxModule.Install(expectedVersion, allUsers);
        }

        private void Register()
        {
            var appxModule = new AppxModuleHelper(this.PsCmdlet);
            appxModule.RegisterAppInstaller();
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
