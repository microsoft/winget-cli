// -----------------------------------------------------------------------------
// <copyright file="PSPackageInstallerType.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    /// <summary>
    /// The installer type of the package.
    /// </summary>
    public enum PSPackageInstallerType
    {
        /// <summary>
        /// Unknown.
        /// </summary>
        Unknown,

        /// <summary>
        /// Inno,
        /// </summary>
        Inno,

        /// <summary>
        /// Wix.
        /// </summary>
        Wix,

        /// <summary>
        /// Msi.
        /// </summary>
        Msi,

        /// <summary>
        /// Nullsoft.
        /// </summary>
        Nullsoft,

        /// <summary>
        /// Zip.
        /// </summary>
        Zip,

        /// <summary>
        /// Msix.
        /// </summary>
        Msix,

        /// <summary>
        /// Exe.
        /// </summary>
        Exe,

        /// <summary>
        /// Burn.
        /// </summary>
        Burn,

        /// <summary>
        /// MSStore,
        /// </summary>
        MSStore,

        /// <summary>
        /// Portable.
        /// </summary>
        Portable,
    }
}