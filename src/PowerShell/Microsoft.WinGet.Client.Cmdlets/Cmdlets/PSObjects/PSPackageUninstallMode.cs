// -----------------------------------------------------------------------------
// <copyright file="PSPackageUninstallMode.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// Must match Microsoft.Management.Deployment.PackageUninstallMode.
    /// </summary>
    public enum PSPackageUninstallMode
    {
        /// <summary>
        /// Default.
        /// </summary>
        Default,

        /// <summary>
        /// Silent.
        /// </summary>
        Silent,

        /// <summary>
        /// Interactive.
        /// </summary>
        Interactive,
    }
}
