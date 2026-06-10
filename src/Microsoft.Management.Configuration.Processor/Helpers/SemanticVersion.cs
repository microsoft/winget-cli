// -----------------------------------------------------------------------------
// <copyright file="SemanticVersion.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Helpers
{
    using System;

    /// <summary>
    /// A semantic version.
    /// </summary>
    internal class SemanticVersion
    {
        private const string MaxRange = "999999999";

        private readonly string semanticVersion;

        /// <summary>
        /// Initializes a new instance of the <see cref="SemanticVersion"/> class.
        /// </summary>
        /// <param name="version">Version.</param>
        public SemanticVersion(string version)
        {
            this.semanticVersion = GetMaximumVersion(version);

            // Prerelease versions append the prerelease tag after a -
            // PowerShell doesn't handle semantic versions.
            if (this.semanticVersion.Contains("-"))
            {
                var indexOf = this.semanticVersion.IndexOf("-");
                this.Version = new Version(this.semanticVersion[..indexOf]);
                this.PrereleaseTag = this.semanticVersion[(indexOf + 1) ..];
            }
            else
            {
                this.Version = new Version(this.semanticVersion);
            }
        }

        /// <summary>
        /// Gets the version without a prerelease tag.
        /// </summary>
        public Version Version { get; }

        /// <summary>
        /// Gets prerelease tag if any.
        /// </summary>
        public string? PrereleaseTag { get; } = null;

        /// <summary>
        /// Gets a value indicating whether if the semantic version is prerelease.
        /// </summary>
        public bool IsPrerelease => !string.IsNullOrEmpty(this.PrereleaseTag);

        /// <inheritdoc/>
        public override string ToString()
        {
            return this.semanticVersion;
        }

        /// <summary>
        /// Max out a version by replacing * if needed.
        /// </summary>
        /// <param name="version">Version.</param>
        /// <returns>Maxed version.</returns>
        private static string GetMaximumVersion(string version)
        {
            return version.Replace("*", MaxRange);
        }
    }
}
