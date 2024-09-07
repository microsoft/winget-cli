// -----------------------------------------------------------------------------
// <copyright file="PSSourceTrustLevel.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Cmdlets.PSObjects
{
    /// <summary>
    /// This is the powershell argument equivalent of AppInstaller::Repository::SourceTrustLevel.
    /// </summary>
    public enum PSSourceTrustLevel
    {
        /// <summary>
        /// Let winget decide.
        /// </summary>
        Default,

        /// <summary>
        /// None.
        /// </summary>
        None,

        /// <summary>
        /// Trusted.
        /// </summary>
        Trusted,
    }
}
