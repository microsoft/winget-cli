// -----------------------------------------------------------------------------
// <copyright file="WinGetPackageNotInstalledException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Exceptions
{
    using System;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// No package found.
    /// </summary>
    [Serializable]
    public class WinGetPackageNotInstalledException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetPackageNotInstalledException"/> class.
        /// </summary>
        public WinGetPackageNotInstalledException()
            : base(Resources.WinGetPackageNotInstalledMessage)
        {
        }
    }
}
