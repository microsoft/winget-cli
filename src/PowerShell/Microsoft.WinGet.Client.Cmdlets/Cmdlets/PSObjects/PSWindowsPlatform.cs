// -----------------------------------------------------------------------------
// <copyright file="PSWindowsPlatform.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// The Windows platform type.
    /// </summary>
    public enum PSWindowsPlatform
    {
        /// <summary>
        /// Let winget decide.
        /// </summary>
        Default,

        /// <summary>
        /// Windows.Universal
        /// </summary>
        Universal,

        /// <summary>
        /// Windows.Desktop
        /// </summary>
        Desktop,

        /// <summary>
        /// Windows.IoT
        /// </summary>
        IoT,

        /// <summary>
        /// Windows.Team
        /// </summary>
        Team,

        /// <summary>
        /// Windows.Holographic
        /// </summary>
        Holographic,
    }
}
