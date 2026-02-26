// -----------------------------------------------------------------------------
// <copyright file="StreamType.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Common.Command
{
    /// <summary>
    /// The write stream type of the cmdlet.
    /// </summary>
    public enum StreamType
    {
        /// <summary>
        /// Debug.
        /// </summary>
        Debug,

        /// <summary>
        /// Verbose.
        /// </summary>
        Verbose,

        /// <summary>
        /// Warning.
        /// </summary>
        Warning,

        /// <summary>
        /// Error.
        /// </summary>
        Error,

        /// <summary>
        /// Progress.
        /// </summary>
        Progress,

        /// <summary>
        /// Object.
        /// </summary>
        Object,

        /// <summary>
        /// Information.
        /// </summary>
        Information,
    }
}
