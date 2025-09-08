// -----------------------------------------------------------------------------
// <copyright file="WinGetPackageManagerCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
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
        /// <param name="includePrerelease">Include prerelease versions when matching version.</param>
        public void Repair(string expectedVersion, bool allUsers, bool force, bool includePrerelease)
        {
            this.ValidateWhenAllUsers(allUsers);
            var runningTask = this.RunOnMTA(
                async () =>
                {
                    if (!string.IsNullOrWhiteSpace(expectedVersion))
                    {
                        this.Write(StreamType.Verbose, $"Attempting to resolve version '{expectedVersion}'");
                        expectedVersion = await this.ResolveVersionAsync(expectedVersion, includePrerelease);
                    }
                    else
                    {
                        this.Write(StreamType.Verbose, "No version specified.");
                    }

                    await this.RepairStateMachineAsync(expectedVersion, allUsers, force);
                    return true;
                });
            this.Wait(runningTask);
        }

        /// <summary>
        /// Tries to get the latest version matching the pattern.
        /// </summary>
        /// <remarks>
        /// Pattern only supports trailing wildcards.
        /// - For example, the pattern can be: 1.11.*, 1.11.3*
        /// - But it cannot be: 1.*1.1 or 1.*1*.1.
        /// </remarks>
        /// <param name="versions">List of versions to match against.</param>
        /// <param name="pattern">Pattern to match.</param>
        /// <param name="includePrerelease">Include prerelease versions.</param>
        /// <param name="result">The resulting version.</param>
        /// <returns>True if a matching version was found.</returns>
        private static bool TryGetLatestMatchingVersion(IEnumerable<WinGetVersion> versions, string pattern, bool includePrerelease, out WinGetVersion result)
        {
            pattern = string.IsNullOrWhiteSpace(pattern) ? "*" : pattern;

            var parts = pattern.Split('.');
            string major = parts[0];
            string minor = parts.Length > 1 ? parts[1] : string.Empty;
            string build = parts.Length > 2 ? parts[2] : string.Empty;
            string revision = parts.Length > 3 ? parts[3] : string.Empty;

            if (!includePrerelease)
            {
                versions = versions.Where(v => !v.IsPrerelease);
            }

            versions = versions
                .Where(v =>
                    VersionPartMatch(major, v.Version.Major) &&
                    VersionPartMatch(minor, v.Version.Minor) &&
                    VersionPartMatch(build, v.Version.Build) &&
                    VersionPartMatch(revision, v.Version.Revision))
                .OrderBy(f => f.Version);

            if (!versions.Any())
            {
                result = null!;
                return false;
            }

            result = versions.Last();
            return true;
        }

        private static bool VersionPartMatch(string partPattern, int partValue)
        {
            if (string.IsNullOrWhiteSpace(partPattern))
            {
                return true;
            }

            if (partPattern.EndsWith("*"))
            {
                return partValue.ToString().StartsWith(partPattern.TrimEnd('*'));
            }

            return partPattern == partValue.ToString();
        }

        private async Task<string> ResolveVersionAsync(string version, bool includePrerelease)
        {
            try
            {
                var gitHubClient = new GitHubClient(RepositoryOwner.Microsoft, RepositoryName.WinGetCli);
                var allReleases = await gitHubClient.GetAllReleasesAsync();
                var allWinGetReleases = allReleases.Select(r => new WinGetVersion(r.TagName));
                if (TryGetLatestMatchingVersion(allWinGetReleases, version, includePrerelease, out var latestVersion))
                {
                    this.Write(StreamType.Verbose, $"Matching version found: {latestVersion.TagVersion}");
                    return latestVersion.TagVersion;
                }

                this.Write(StreamType.Warning, $"No matching version found for {version}");
                return version;
            }
            catch (Exception e)
            {
                this.Write(StreamType.Warning, $"Could not resolve version '{version}': {e.Message}");
                return version;
            }
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
                            await this.RegisterAsync(expectedVersion, allUsers);
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
                        case IntegrityCategory.WinGetSourceNotInstalled:
                            await this.InstallWinGetSourceAsync();
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

        private async Task InstallWinGetSourceAsync()
        {
            this.Write(StreamType.Verbose, "Installing winget source");
            var appxModule = new AppxModuleHelper(this);
            await appxModule.InstallWinGetSourceAsync();
        }

        private async Task RegisterAsync(string toRegisterVersion, bool allUsers)
        {
            var appxModule = new AppxModuleHelper(this);
            await appxModule.RegisterAppInstallerAsync(toRegisterVersion, allUsers);
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
