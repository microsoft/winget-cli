// -----------------------------------------------------------------------------
// <copyright file="RepairResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    /// <summary>
    /// Repair result.
    /// </summary>
    public enum RepairResult
    {
        /// <summary>
        /// Failure.
        /// </summary>
        Failure,

        /// <summary>
        /// No changes were made.
        /// </summary>
        Noop,

        /// <summary>
        /// WinGet installed.
        /// </summary>
        Installed,

        /// <summary>
        /// WinGet registered.
        /// </summary>
        Registered,

        /// <summary>
        /// WinGet updated.
        /// </summary>
        Updated,

        /// <summary>
        /// WinGet downgraded.
        /// </summary>
        Downgraded,

        /// <summary>
        /// Path environment fixed.
        /// </summary>
        PathUpdated,

        /// <summary>
        /// NeedsManual repair.
        /// </summary>
        NeedsManualRepair,
    }
}
