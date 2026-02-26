// -----------------------------------------------------------------------------
// <copyright file="PackageResponse.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    using Microsoft.Management.Deployment;

    /// <summary>
    /// Contains information relevant to a failure to identify a package.
    /// </summary>
    internal class PackageIdentityErrorResult
    {
        public required string Message { get; init; }

        public required string Identifier { get; init; }

        public string? Source { get; set; }

        public List<FindPackageResult> Packages { get; set; } = new List<FindPackageResult>();
    }
}
