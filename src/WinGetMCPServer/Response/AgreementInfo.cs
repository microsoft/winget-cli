// -----------------------------------------------------------------------------
// <copyright file="AgreementInfo.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    /// <summary>
    /// An agreement that must be presented to the user.
    /// </summary>
    internal class AgreementInfo
    {
        /// <summary>
        /// The agreement is required by a package source.
        /// </summary>
        public const string SourceKind = "source";

        /// <summary>
        /// The agreement is required by a package.
        /// </summary>
        public const string PackageKind = "package";

        /// <summary>
        /// Gets whether the agreement is required by a source or a package.
        /// </summary>
        public required string Kind { get; init; }

        /// <summary>
        /// Gets the name of the source that requires the agreement, or that provides the package.
        /// </summary>
        public string? Source { get; init; }

        /// <summary>
        /// Gets the identifier of the package that requires the agreement.
        /// </summary>
        public string? Package { get; init; }

        public string? Label { get; init; }

        public string? Text { get; init; }

        public string? Url { get; init; }
    }
}
