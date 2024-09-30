// -----------------------------------------------------------------------------
// <copyright file="GitHubClient.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System.Threading.Tasks;
    using Octokit;

    /// <summary>
    /// Handles GitHub interactions.
    /// </summary>
    internal class GitHubClient
    {
        private readonly string owner;
        private readonly string repo;
        private readonly IGitHubClient gitHubClient;

        /// <summary>
        /// Initializes a new instance of the <see cref="GitHubClient"/> class.
        /// </summary>
        /// <param name="owner">Owner.</param>
        /// <param name="repo">Repository.</param>
        public GitHubClient(string owner, string repo)
        {
            this.gitHubClient = new Octokit.GitHubClient(new ProductHeaderValue(HttpClientHelper.UserAgent));
            this.owner = owner;
            this.repo = repo;
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
            Release release;

            // GetLatest doesn't respect prerelease or gives an option to get it.
            if (includePrerelease)
            {
                // GetAll orders by newest and includes pre releases.
                release = (await this.gitHubClient.Repository.Release.GetAll(this.owner, this.repo))[0];
            }
            else
            {
                release = await this.gitHubClient.Repository.Release.GetLatest(this.owner, this.repo);
            }

            return release;
        }
    }
}
