// -----------------------------------------------------------------------------
// <copyright file="PSPackageInstallScope.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// This must match Microsoft.Management.Deployment.PackageInstallScope.
    /// </summary>
    public enum PSPackageInstallScope
    {
        /// <summary>
        /// Any.
        /// </summary>
        Any,

        /// <summary>
        /// User.
        /// </summary>
        User,

        /// <summary>
        /// System.
        /// </summary>
        System,

        /// <summary>
        /// User or unknown.
        /// </summary>
        UserOrUnknown,

        /// <summary>
        /// System or unknown.
        /// </summary>
        SystemOrUnknown,
    }
}
