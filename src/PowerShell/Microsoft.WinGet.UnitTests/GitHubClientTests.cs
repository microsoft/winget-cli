// -----------------------------------------------------------------------------
// <copyright file="GitHubClientTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.UnitTests
{
    using System;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Xunit;

    /// <summary>
    /// Tests for <see cref="GitHubClient"/> token resolution.
    /// </summary>
    public class GitHubClientTests : IDisposable
    {
        private readonly string? originalGhToken;
        private readonly string? originalGithubToken;

        /// <summary>
        /// Initializes a new instance of the <see cref="GitHubClientTests"/> class.
        /// </summary>
        public GitHubClientTests()
        {
            this.originalGhToken = Environment.GetEnvironmentVariable("GH_TOKEN");
            this.originalGithubToken = Environment.GetEnvironmentVariable("GITHUB_TOKEN");
        }

        /// <summary>
        /// Tests that GH_TOKEN takes precedence when both tokens are set.
        /// </summary>
        [Fact]
        public void ResolveGitHubToken_BothSet_PrefersGhToken()
        {
            Environment.SetEnvironmentVariable("GH_TOKEN", "gh-token-value");
            Environment.SetEnvironmentVariable("GITHUB_TOKEN", "github-token-value");

            string? result = GitHubClient.ResolveGitHubToken();

            Assert.Equal("gh-token-value", result);
        }

        /// <summary>
        /// Tests that GITHUB_TOKEN is used when GH_TOKEN is not set.
        /// </summary>
        [Fact]
        public void ResolveGitHubToken_OnlyGithubToken_ReturnsGithubToken()
        {
            Environment.SetEnvironmentVariable("GH_TOKEN", null);
            Environment.SetEnvironmentVariable("GITHUB_TOKEN", "github-token-value");

            string? result = GitHubClient.ResolveGitHubToken();

            Assert.Equal("github-token-value", result);
        }

        /// <summary>
        /// Tests that GH_TOKEN is used when GITHUB_TOKEN is not set.
        /// </summary>
        [Fact]
        public void ResolveGitHubToken_OnlyGhToken_ReturnsGhToken()
        {
            Environment.SetEnvironmentVariable("GH_TOKEN", "gh-token-value");
            Environment.SetEnvironmentVariable("GITHUB_TOKEN", null);

            string? result = GitHubClient.ResolveGitHubToken();

            Assert.Equal("gh-token-value", result);
        }

        /// <summary>
        /// Tests that null is returned when no tokens are set.
        /// </summary>
        [Fact]
        public void ResolveGitHubToken_NoneSet_ReturnsNull()
        {
            Environment.SetEnvironmentVariable("GH_TOKEN", null);
            Environment.SetEnvironmentVariable("GITHUB_TOKEN", null);

            string? result = GitHubClient.ResolveGitHubToken();

            Assert.Null(result);
        }

        /// <summary>
        /// Tests that whitespace-only tokens are treated as not set.
        /// </summary>
        [Fact]
        public void ResolveGitHubToken_WhitespaceTokens_ReturnsNull()
        {
            Environment.SetEnvironmentVariable("GH_TOKEN", "   ");
            Environment.SetEnvironmentVariable("GITHUB_TOKEN", "  ");

            string? result = GitHubClient.ResolveGitHubToken();

            Assert.Null(result);
        }

        /// <summary>
        /// Tests that GITHUB_TOKEN is used when GH_TOKEN is whitespace.
        /// </summary>
        [Fact]
        public void ResolveGitHubToken_GhTokenWhitespace_FallsBackToGithubToken()
        {
            Environment.SetEnvironmentVariable("GH_TOKEN", "  ");
            Environment.SetEnvironmentVariable("GITHUB_TOKEN", "github-token-value");

            string? result = GitHubClient.ResolveGitHubToken();

            Assert.Equal("github-token-value", result);
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            Environment.SetEnvironmentVariable("GH_TOKEN", this.originalGhToken);
            Environment.SetEnvironmentVariable("GITHUB_TOKEN", this.originalGithubToken);
        }
    }
}
