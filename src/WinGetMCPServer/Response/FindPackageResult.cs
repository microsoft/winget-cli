// -----------------------------------------------------------------------------
// <copyright file="FindPackageResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    /// <summary>
    /// Contains information about found packages.
    /// </summary>
    internal class FindPackageResult
    {
        public required string Identifier { get; init; }

        public string? Name { get; set; }

        public string? Source { get; set; }

        public bool? IsInstalled { get; set; }

        public string? InstalledVersion { get; set; }

        public string? InstalledLocation { get; set; }

        public bool? IsUpdateAvailable { get; set; }
    }
}
