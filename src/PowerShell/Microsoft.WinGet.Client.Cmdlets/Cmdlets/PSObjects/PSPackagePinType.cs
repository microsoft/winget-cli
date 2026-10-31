// -----------------------------------------------------------------------------
// <copyright file="PSPackagePinType.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// Must match the user-settable values of Microsoft.Management.Deployment.PackagePinType.
    /// </summary>
    public enum PSPackagePinType
    {
        /// <summary>
        /// Pinning - prevents automatic updates to the current version.
        /// </summary>
        Pinning,

        /// <summary>
        /// Blocking - prevents installation or upgrade of the package.
        /// </summary>
        Blocking,

        /// <summary>
        /// Gating - limits updates to versions below the specified GatedVersion.
        /// </summary>
        Gating,
    }
}
