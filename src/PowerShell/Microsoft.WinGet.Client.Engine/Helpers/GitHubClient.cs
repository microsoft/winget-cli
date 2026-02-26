// -----------------------------------------------------------------------------
// <copyright file="GitHubClient.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading.Tasks;
    using Microsoft.WinGet.Common.Command;
    using Octokit;
    using static Microsoft.WinGet.Client.Engine.Common.Constants;

    /// <summary>
    /// Handles GitHub interactions.
    /// </summary>
    internal class GitHubClient
    {
        private readonly string owner;
        private readonly string repo;
        private readonly IGitHubClient gitHubClient;
        private readonly PowerShellCmdlet? pwshCmdlet;

        /// <summary>
        /// Initializes a new instance of the <see cref="GitHubClient"/> class.
        /// </summary>
        /// <param name="owner">Owner.</param>
        /// <param name="repo">Repository.</param>
        /// <param name="pwshCmdlet">Optional PowerShell cmdlet for logging.</param>
        public GitHubClient(string owner, string repo, PowerShellCmdlet? pwshCmdlet = null)
        {
            this.pwshCmdlet = pwshCmdlet;
            var octokitClient = new Octokit.GitHubClient(new ProductHeaderValue(HttpClientHelper.UserAgent));

            string? token = ResolveGitHubToken(pwshCmdlet);
            if (!string.IsNullOrWhiteSpace(token))
            {
                octokitClient.Credentials = new Credentials(token);
            }

            this.gitHubClient = octokitClient;
            this.owner = owner;
            this.repo = repo;
        }

        /// <summary>
        /// Reads all known GitHub token environment variables, logs their presence,
        /// and selects the one to use based on precedence.
        /// GH_TOKEN takes precedence over GITHUB_TOKEN, matching GitHub CLI behavior.
        /// See: https://cli.github.com/manual/gh_help_environment.
        /// </summary>
        /// <param name="pwshCmdlet">Optional PowerShell cmdlet for logging.</param>
        /// <returns>The selected token value, or null if none found.</returns>
        internal static string? ResolveGitHubToken(PowerShellCmdlet? pwshCmdlet = null)
        {
            string? ghToken = Environment.GetEnvironmentVariable("GH_TOKEN");
            string? githubToken = Environment.GetEnvironmentVariable("GITHUB_TOKEN");

            bool hasGhToken = !string.IsNullOrWhiteSpace(ghToken);
            bool hasGithubToken = !string.IsNullOrWhiteSpace(githubToken);

            pwshCmdlet?.Write(StreamType.Verbose, $"GH_TOKEN environment variable: {(hasGhToken ? "found" : "not found")}");
            pwshCmdlet?.Write(StreamType.Verbose, $"GITHUB_TOKEN environment variable: {(hasGithubToken ? "found" : "not found")}");

            if (hasGhToken)
            {
                pwshCmdlet?.Write(StreamType.Verbose, "Using authenticated GitHub API requests via GH_TOKEN environment variable.");
                return ghToken;
            }
            else if (hasGithubToken)
            {
                pwshCmdlet?.Write(StreamType.Verbose, "Using authenticated GitHub API requests via GITHUB_TOKEN environment variable.");
                return githubToken;
            }

            pwshCmdlet?.Write(StreamType.Verbose, "No GitHub token found. Using unauthenticated GitHub API requests.");
            return null;
        }

        /// <summary>
        /// Gets a release.
        /// </summary>
        /// <param name="releaseTag">Release tag.</param>
        /// <returns>The Release.</returns>
        public async Task<Release> GetReleaseAsync(string releaseTag)
        {
            return await this.gitHubClient.Repository.Release.Get(this.owner, this.repo, releaseTag);
        }

        /// <summary>
        /// Gets the latest released and waits.
        /// </summary>
        /// <param name="includePrerelease">Include prerelease.</param>
        /// <returns>Latest version.</returns>
        public async Task<string> GetLatestReleaseTagNameAsync(bool includePrerelease)
        {
            return (await this.GetLatestReleaseAsync(includePrerelease)).TagName;
        }

        /// <summary>
        /// Gets the latest released version.
        /// </summary>
        /// <param name="includePrerelease">Include prerelease.</param>
        /// <returns>Latest version.</returns>
        public async Task<Release> GetLatestReleaseAsync(bool includePrerelease)
        {
            var allReleases = await this.GetAllReleasesAsync();
            allReleases = includePrerelease ? allReleases : allReleases.Where(r => !r.Prerelease).ToList();
            return allReleases.Select(r => new { Release = r, WinGetVersion = new WinGetVersion(r.TagName) })
                .OrderBy(rv => rv.WinGetVersion.Version)
                .Last().Release;
        }

        /// <summary>
        /// Gets all releases.
        /// </summary>
        /// <returns>All releases.</returns>
        public async Task<IReadOnlyList<Release>> GetAllReleasesAsync()
        {
            return await this.gitHubClient.Repository.Release.GetAll(this.owner, this.repo);
        }

        /// <summary>
        /// Resolve a version string to the latest matching version from GitHub releases.
        /// </summary>
        /// <param name="version">Version string to resolve. Can include wildcards (*).</param>
        /// <param name="includePrerelease">Whether to include prerelease versions in the search.</param>
        /// <returns>Resolved version string or null if no match found.</returns>
        public async Task<string?> ResolveVersionAsync(string version, bool includePrerelease)
        {
            if (!WinGetVersion.VersionHasWildcard(version))
            {
                return version;
            }

            var allReleases = await this.GetAllReleasesAsync();
            var allWinGetReleases = allReleases.Select(r => new WinGetVersion(r.TagName));
            if (TryGetLatestMatchingVersion(allWinGetReleases, version, includePrerelease, out var latestVersion))
            {
                return latestVersion!.TagVersion;
            }

            return null;
        }

        /// <summary>
        /// Tries to get the latest version matching the pattern.
        /// </summary>
        /// <remarks>
        /// Pattern only supports leading and trailing wildcards.
        /// - For example, the pattern can be: 1.11.*, 1.11.3*, 1.11.*3
        /// - But it cannot be: 1.*1*.1 or 1.1*1.1.
        /// </remarks>
        /// <param name="versions">List of versions to match against.</param>
        /// <param name="pattern">Pattern to match.</param>
        /// <param name="includePrerelease">Include prerelease versions.</param>
        /// <param name="result">The resulting version.</param>
        /// <returns>True if a matching version was found.</returns>
        private static bool TryGetLatestMatchingVersion(IEnumerable<WinGetVersion> versions, string pattern, bool includePrerelease, out WinGetVersion? result)
        {
            pattern = string.IsNullOrWhiteSpace(pattern) ? "*" : pattern;

            var parts = pattern.Split('.');
            var major = parts.ElementAtOrDefault(0);
            var minor = parts.ElementAtOrDefault(1);
            var build = parts.ElementAtOrDefault(2);
            var revision = parts.ElementAtOrDefault(3);

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

        /// <summary>
        /// Checks if a version part matches a pattern.
        /// </summary>
        /// <param name="partPattern">Version part pattern.</param>
        /// <param name="partValue">Version part value.</param>
        /// <returns>True if the part matches the pattern.</returns>
        private static bool VersionPartMatch(string? partPattern, int partValue)
        {
            if (string.IsNullOrWhiteSpace(partPattern))
            {
                return true;
            }

            if (partPattern!.StartsWith("*"))
            {
                return partValue.ToString().EndsWith(partPattern.TrimStart('*'));
            }

            if (partPattern!.EndsWith("*"))
            {
                return partValue.ToString().StartsWith(partPattern.TrimEnd('*'));
            }

            return partPattern == partValue.ToString();
        }
    }
}
