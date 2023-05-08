// -----------------------------------------------------------------------------
// <copyright file="PSProcessorArchitecture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// The processor architecture of the package to install.
    /// </summary>
    public enum PSProcessorArchitecture
    {
        /// <summary>
        /// Let winget decide.
        /// </summary>
        Default,

        /// <summary>
        /// x86,
        /// </summary>
        X86,

        /// <summary>
        /// ARM.
        /// </summary>
        Arm,

        /// <summary>
        /// x64.
        /// </summary>
        X64,

        /// <summary>
        /// Arm64
        /// </summary>
        Arm64,
    }
}
