// -----------------------------------------------------------------------------
// <copyright file="PSPackageInstallMode.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// This should mimic Microsoft.Management.Deployment.PackageInstallMode.
    /// </summary>
    public enum PSPackageInstallMode
    {
        /// <summary>
        /// Default,
        /// </summary>
        Default,

        /// <summary>
        /// Silent,
        /// </summary>
        Silent,

        /// <summary>
        /// Interactive,
        /// </summary>
        Interactive,
    }
}
