// -----------------------------------------------------------------------------
// <copyright file="PSApplyConfigurationUnitResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Helpers;

    /// <summary>
    /// The apply result of a configuration unit.
    /// </summary>
    public class PSApplyConfigurationUnitResult : PSUnitResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSApplyConfigurationUnitResult"/> class.
        /// </summary>
        /// <param name="unitResult">Apply unit result.</param>
        internal PSApplyConfigurationUnitResult(ApplyConfigurationUnitResult unitResult)
            : base(unitResult.Unit, unitResult.ResultInformation)
        {
            this.State = Utilities.ToPSConfigurationUnitState(unitResult.State);
            this.PreviouslyInDesiredState = unitResult.PreviouslyInDesiredState;
        }

        /// <summary>
        /// Gets the unit state.
        /// </summary>
        public PSConfigurationUnitState State { get; private init; }

        /// <summary>
        /// Gets a value indicating whether the unit was in a previous desired state.
        /// </summary>
        public bool PreviouslyInDesiredState { get; private init; }

        /// <summary>
        /// Gets a value indicating whether a reboot is required after the configuration unit was applied.
        /// </summary>
        public bool RebootRequired { get; private init; }
    }
}
