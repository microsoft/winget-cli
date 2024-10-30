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
    using System.Threading.Tasks;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;
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
            var runningTask = this.RunOnMTA(
                async () =>
                {
                    var gitHubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
                    string expectedVersion = await gitHubClient.GetLatestReleaseTagNameAsync(preRelease);
                    this.Assert(expectedVersion);
                    return true;
                });

            this.Wait(runningTask);
        }

        /// <summary>
        /// Asserts the version installed is the specified.
        /// </summary>
        /// <param name="expectedVersion">The expected version.</param>
        public void Assert(string expectedVersion)
        {
            WinGetIntegrity.AssertWinGet(this, expectedVersion);
        }

        /// <summary>
        /// Repairs winget using the latest version on winget-cli.
        /// </summary>
        /// <param name="preRelease">Use prerelease version on GitHub.</param>
        /// <param name="allUsers">Install for all users. Requires admin.</param>
        /// <param name="force">Force application shutdown.</param>
        public void RepairUsingLatest(bool preRelease, bool allUsers, bool force)
        {
            this.ValidateWhenAllUsers(allUsers);
            var runningTask = this.RunOnMTA(
                async () =>
                {
                    var gitHubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
                    string expectedVersion = await gitHubClient.GetLatestReleaseTagNameAsync(preRelease);
                    await this.RepairStateMachineAsync(expectedVersion, allUsers, force);
                    return true;
                });

            this.Wait(runningTask);
        }

        /// <summary>
        /// Repairs winget if needed.
        /// </summary>
        /// <param name="expectedVersion">The expected version, if any.</param>
        /// <param name="allUsers">Install for all users. Requires admin.</param>
        /// <param name="force">Force application shutdown.</param>
        public void Repair(string expectedVersion, bool allUsers, bool force)
        {
            this.ValidateWhenAllUsers(allUsers);
            var runningTask = this.RunOnMTA(
                async () =>
                {
                    await this.RepairStateMachineAsync(expectedVersion, allUsers, force);
                    return true;
                });
            this.Wait(runningTask);
        }

        private async Task RepairStateMachineAsync(string expectedVersion, bool allUsers, bool force)
        {
            var seenCategories = new HashSet<IntegrityCategory>();
            var cancellationToken = this.GetCancellationToken();

            var currentCategory = IntegrityCategory.Unknown;
            while (currentCategory != IntegrityCategory.Installed)
            {
                cancellationToken.ThrowIfCancellationRequested();

                try
                {
                    WinGetIntegrity.AssertWinGet(this, expectedVersion);
                    this.Write(StreamType.Verbose, $"WinGet is in a good state.");
                    currentCategory = IntegrityCategory.Installed;
                }
                catch (WinGetIntegrityException e)
                {
                    currentCategory = e.Category;

                    if (seenCategories.Contains(currentCategory))
                    {
                        this.Write(StreamType.Verbose, $"{currentCategory} encountered previously");
                        throw;
                    }

                    this.Write(StreamType.Verbose, $"Integrity category type: {currentCategory}");
                    seenCategories.Add(currentCategory);

                    switch (currentCategory)
                    {
                        case IntegrityCategory.UnexpectedVersion:
                            await this.InstallDifferentVersionAsync(new WinGetVersion(expectedVersion), allUsers, force);
                            break;
                        case IntegrityCategory.NotInPath:
                            this.RepairEnvPath();
                            break;
                        case IntegrityCategory.AppInstallerNotRegistered:
                            this.Register(expectedVersion);
                            break;
                        case IntegrityCategory.AppInstallerNotInstalled:
                        case IntegrityCategory.AppInstallerNotSupported:
                        case IntegrityCategory.Failure:
                            await this.InstallAsync(expectedVersion, allUsers, force);
                            break;
                        case IntegrityCategory.AppInstallerNoLicense:
                            // This requires -AllUsers in admin mode.
                            if (allUsers && Utilities.ExecutingAsAdministrator)
                            {
                                await this.InstallAsync(expectedVersion, allUsers, force);
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

        private async Task InstallDifferentVersionAsync(WinGetVersion toInstallVersion, bool allUsers, bool force)
        {
            var installedVersion = WinGetVersion.InstalledWinGetVersion(this);
            bool isDowngrade = installedVersion.CompareAsDeployment(toInstallVersion) > 0;

            string message = $"Installed WinGet version '{installedVersion.TagVersion}' " +
                $"Installing WinGet version '{toInstallVersion.TagVersion}' " +
                $"Is downgrade {isDowngrade}";
            this.Write(
                StreamType.Verbose,
                message);
            var appxModule = new AppxModuleHelper(this);
            await appxModule.InstallFromGitHubReleaseAsync(toInstallVersion.TagVersion, allUsers, isDowngrade, force);
        }

        private async Task InstallAsync(string toInstallVersion, bool allUsers, bool force)
        {
            // If we are here and toInstallVersion is empty, it means that they just ran Repair-WinGetPackageManager.
            // When there is not version specified, we don't want to assume an empty version means latest, but in
            // this particular case we need to.
            if (string.IsNullOrEmpty(toInstallVersion))
            {
                var gitHubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
                toInstallVersion = await gitHubClient.GetLatestReleaseTagNameAsync(false);
            }

            var appxModule = new AppxModuleHelper(this);
            await appxModule.InstallFromGitHubReleaseAsync(toInstallVersion, allUsers, false, force);
        }

        private void Register(string toRegisterVersion)
        {
            var appxModule = new AppxModuleHelper(this);
            appxModule.RegisterAppInstaller(toRegisterVersion);
        }

        private void RepairEnvPath()
        {
            // Add windows app path to user PATH environment variable
            Utilities.AddWindowsAppToPath();

            // Update this sessions PowerShell environment so the user doesn't have to restart the terminal.
            string? envPathUser = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.User);
            string? envPathMachine = Environment.GetEnvironmentVariable(Constants.PathEnvVar, EnvironmentVariableTarget.Machine);
            string newPwshPathEnv = $"{envPathMachine};{envPathUser}";
            this.SetVariable(EnvPath, newPwshPathEnv);

            this.Write(StreamType.Verbose, $"PATH environment variable updated");
        }

        private void ValidateWhenAllUsers(bool allUsers)
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
        }
    }
}
