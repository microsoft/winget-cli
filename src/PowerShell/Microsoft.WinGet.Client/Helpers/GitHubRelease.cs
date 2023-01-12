// -----------------------------------------------------------------------------
// <copyright file="GitHubRelease.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading.Tasks;
    using Octokit;
    using FileMode = System.IO.FileMode;

    /// <summary>
    /// Handles WinGet's releases in GitHub.
    /// </summary>
    internal class GitHubRelease
    {
        private const string Owner = "microsoft";
        private const string Repo = "winget-cli";
        private const string UserAgent = "winget-powershell";
        private const string MsixBundleName = "Microsoft.DesktopAppInstaller_8wekyb3d8bbwe.msixbundle";
        private const string ContentType = "application/octet-stream";

        private readonly IGitHubClient gitHubClient;

        /// <summary>
        /// Initializes a new instance of the <see cref="GitHubRelease"/> class.
        /// </summary>
        public GitHubRelease()
        {
            this.gitHubClient = new GitHubClient(new ProductHeaderValue(UserAgent));
        }

        /// <summary>
        /// Download a release from winget-cli.
        /// </summary>
        /// <param name="releaseTag">Optional release name. If null, gets latest.</param>
        /// <returns>Path where the msix bundle is downloaded.</returns>
        public string DownloadRelease(string releaseTag)
        {
            return this.DownloadReleaseAsync(releaseTag).GetAwaiter().GetResult();
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
        /// Download asynchronously a release from winget-cli.
        /// </summary>
        /// <param name="releaseTag">Optional release name. If null, gets latest.</param>
        /// <returns>Path where the msix bundle is downloaded.</returns>
        public async Task<string> DownloadReleaseAsync(string releaseTag)
        {
            Release release = await this.gitHubClient.Repository.Release.Get(Owner, Repo, releaseTag);

            // Get asset and download.
            var msixBundleAsset = release.Assets.Where(a => a.Name == MsixBundleName).First();

            var tmpFile = Path.GetTempFileName();
            await this.DownloadUrlAsync(msixBundleAsset.Url, tmpFile);
            return tmpFile;
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
            using var fileStream = File.Open(fileName, FileMode.Open);
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
                release = (await this.gitHubClient.Repository.Release.GetAll(Owner, Repo))[0];
            }
            else
            {
                release = await this.gitHubClient.Repository.Release.GetLatest(Owner, Repo);
            }

            return release;
        }
    }
}
