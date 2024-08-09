// -----------------------------------------------------------------------------
// <copyright file="PSConfigurationUnitState.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    /// <summary>
    /// Must match ConfigurationUnitState.
    /// </summary>
    public enum PSConfigurationUnitState
    {
        /// <summary>
        /// The state of the configuration unit is unknown.
        /// </summary>
        Unknown,

        /// <summary>
        /// The configuration unit is in the queue to be applied.
        /// </summary>
        Pending,

        /// <summary>
        /// The configuration unit is actively being applied.
        /// </summary>
        InProgress,

        /// <summary>
        /// The configuration unit has completed being applied.
        /// </summary>
        Completed,

        /// <summary>
        /// The configuration unit was not applied due to external factors.
        /// </summary>
        Skipped,
    }
}
