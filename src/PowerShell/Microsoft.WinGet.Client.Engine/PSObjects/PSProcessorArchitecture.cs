// -----------------------------------------------------------------------------
// <copyright file="PSProcessorArchitecture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    /// <summary>
    /// Must match Windows.System.ProcessorArchitecture except None.
    /// </summary>
    public enum PSProcessorArchitecture
    {
        /// <summary>
        /// None architecture was give.
        /// </summary>
        None,

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
        /// Neutral.
        /// </summary>
        Neutral,

        /// <summary>
        /// Arm64
        /// </summary>
        Arm64,

        /// <summary>
        /// x86OnArm64.
        /// </summary>
        X86OnArm64,
    }
}
