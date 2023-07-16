// -----------------------------------------------------------------------------
// <copyright file="GitHubClient.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Threading.Tasks;
    using Octokit;
    using FileMode = System.IO.FileMode;

    /// <summary>
    /// Handles GitHub interactions.
    /// </summary>
    internal class GitHubClient
    {
        private const string UserAgent = "winget-powershell";
        private const string ContentType = "application/octet-stream";

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
            this.gitHubClient = new Octokit.GitHubClient(new ProductHeaderValue(UserAgent));
            this.owner = owner;
            this.repo = repo;
        }

        /// <summary>
        /// Gets a release.
        /// </summary>
        /// <param name="releaseTag">Release tag.</param>
        /// <returns>The Release.</returns>
        public Release GetRelease(string releaseTag)
        {
            return this.GetReleaseAsync(releaseTag).GetAwaiter().GetResult();
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
        /// Gets the latest released version and waits.
        /// </summary>
        /// <param name="includePreRelease">Include prerelease.</param>
        /// <returns>Latest version.</returns>
        public string GetLatestVersionTagName(bool includePreRelease)
        {
            return this.GetLatestVersionAsync(includePreRelease).GetAwaiter().GetResult().TagName;
        }

        /// <summary>
        /// Downloads a file from a url and waits.
        /// </summary>
        /// <param name="url">Url.</param>
        /// <param name="fileName">File name.</param>
        public void DownloadUrl(string url, string fileName)
        {
            this.DownloadUrlAsync(url, fileName).GetAwaiter().GetResult();
        }

        /// <summary>
        /// Downloads a file from a url.
        /// </summary>
        /// <param name="url">Url.</param>
        /// <param name="fileName">File name.</param>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public async Task DownloadUrlAsync(string url, string fileName)
        {
            var response = await this.gitHubClient.Connection.Get<object>(
                new Uri(url),
                new Dictionary<string, string>(),
                ContentType);

            using var memoryStream = new MemoryStream((byte[])response.Body);
            using var fileStream = File.Open(fileName, FileMode.OpenOrCreate);
            memoryStream.Position = 0;
            await memoryStream.CopyToAsync(fileStream);
        }

        /// <summary>
        /// Gets the latest released version.
        /// </summary>
        /// <param name="includePreRelease">Include prerelease.</param>
        /// <returns>Latest version.</returns>
        internal async Task<Release> GetLatestVersionAsync(bool includePreRelease)
        {
            Release release;

            // GetLatest doesn't respect prerelease or gives an option to get it.
            if (includePreRelease)
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
