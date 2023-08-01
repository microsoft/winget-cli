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
    using static Microsoft.WinGet.Client.Engine.Common.Constants;

    /// <summary>
    /// Used by Repair-WinGetPackageManager and Assert-WinGetPackageManager.
    /// </summary>
    public sealed class WinGetPackageManagerCommand : BaseCommand
    {
        private const string EnvPath = "env:PATH";

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
            var gitHubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
            string expectedVersion = gitHubClient.GetLatestVersionTagName(preRelease);
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
            var gitHubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
            string expectedVersion = gitHubClient.GetLatestVersionTagName(preRelease);
            this.Repair(expectedVersion, allUsers);
        }

        /// <summary>
        /// Repairs winget if needed.
        /// </summary>
        /// <param name="expectedVersion">The expected version, if any.</param>
        /// <param name="allUsers">Install for all users. Requires admin.</param>
        public void Repair(string expectedVersion, bool allUsers)
        {
            if (allUsers)
            {
                if (Utilities.ExecutingAsSystem)
                {
                    throw new NotSupportedException();
                }

                if (!Utilities.ExecutingAsAdministrator)
                {
                    throw new WinGetRepairException(Resources.RepairAllUsersMessage);
                }
            }

            this.RepairStateMachine(expectedVersion, allUsers);
        }

        private void RepairStateMachine(string expectedVersion, bool allUsers)
        {
            var seenCategories = new HashSet<IntegrityCategory>();

            var currentCategory = IntegrityCategory.Unknown;
            while (currentCategory != IntegrityCategory.Installed)
            {
                try
                {
                    WinGetIntegrity.AssertWinGet(this.PsCmdlet, expectedVersion);
                    this.PsCmdlet.WriteDebug($"WinGet is in a good state.");
                    currentCategory = IntegrityCategory.Installed;
                }
                catch (WinGetIntegrityException e)
                {
                    currentCategory = e.Category;

                    if (seenCategories.Contains(currentCategory))
                    {
                        this.PsCmdlet.WriteDebug($"{currentCategory} encountered previously");
                        throw;
                    }

                    this.PsCmdlet.WriteDebug($"Integrity category type: {currentCategory}");
                    seenCategories.Add(currentCategory);

                    switch (currentCategory)
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
                            // This requires -AllUsers in admin mode.
                            if (allUsers && Utilities.ExecutingAsAdministrator)
                            {
                                this.Install(expectedVersion, allUsers);
                            }
                            else
                            {
                                throw new WinGetRepairException(e);
                            }

                            break;
                        case IntegrityCategory.AppExecutionAliasDisabled:
                        case IntegrityCategory.Unknown:
                            throw new WinGetRepairException(e);
                        default:
                            throw new NotSupportedException();
                    }
                }
            }
        }

        private void InstallDifferentVersion(WinGetVersion toInstallVersion, bool allUsers)
        {
            var installedVersion = WinGetVersion.InstalledWinGetVersion;
            bool isDowngrade = installedVersion.CompareAsDeployment(toInstallVersion) > 0;

            this.PsCmdlet.WriteDebug($"Installed WinGet version '{installedVersion.TagVersion}' " +
                $"Installing WinGet version '{toInstallVersion.TagVersion}' " +
                $"Is downgrade {isDowngrade}");
            var appxModule = new AppxModuleHelper(this.PsCmdlet);
            appxModule.InstallFromGitHubRelease(toInstallVersion.TagVersion, allUsers, isDowngrade);
        }

        private void Install(string toInstallVersion, bool allUsers)
        {
            // If we are here and toInstallVersion is empty, it means that they just ran Repair-WinGetPackageManager.
            // When there is not version specified, we don't want to assume an empty version means latest, but in
            // this particular case we need to.
            if (string.IsNullOrEmpty(toInstallVersion))
            {
                var gitHubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
                toInstallVersion = gitHubClient.GetLatestVersionTagName(false);
            }

            var appxModule = new AppxModuleHelper(this.PsCmdlet);
            appxModule.InstallFromGitHubRelease(toInstallVersion, allUsers, false);
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
