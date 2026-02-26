// -----------------------------------------------------------------------------
// <copyright file="InstallOperationResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    /// <summary>
    /// Contains information about an install operation.
    /// </summary>
    internal class InstallOperationResult
    {
        public string? Message { get; set; }

        public bool? RebootRequired { get; set; }

        public int? ErrorCode { get; set; }

        public uint? InstallerErrorCode { get; set; }

        public FindPackageResult? InstalledPackageInformation { get; set; }
    }
}
