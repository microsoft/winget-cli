// -----------------------------------------------------------------------------
// <copyright file="IntegrityCategory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Common
{
    /// <summary>
    /// The type of the integrity check failure.
    /// </summary>
    public enum IntegrityCategory
    {
        /// <summary>
        /// WinGet is correctly installed.
        /// </summary>
        Installed,

        /// <summary>
        /// The version installed is not what is expected.
        /// </summary>
        UnexpectedVersion,

        /// <summary>
        /// Unknown reason.
        /// </summary>
        Unknown,

        /// <summary>
        /// A failure resulted on a simple winget command that shouldn't happen.
        /// </summary>
        Failure,

        /// <summary>
        /// WindowsAppPath not in PATH environment variable.
        /// </summary>
        NotInPath,

        /// <summary>
        /// Winget's app execution alias disabled.
        /// </summary>
        AppExecutionAliasDisabled,

        /// <summary>
        /// Windows OS is not supported.
        /// </summary>
        OsNotSupported,

        /// <summary>
        /// AppInstaller package is not installed.
        /// </summary>
        AppInstallerNotInstalled,

        /// <summary>
        /// AppInstaller package is not registered.
        /// </summary>
        AppInstallerNotRegistered,

        /// <summary>
        /// Installed App Installer package is not supported.
        /// </summary>
        AppInstallerNotSupported,

        /// <summary>
        /// No applicable license found.
        /// </summary>
        AppInstallerNoLicense,
    }
}
