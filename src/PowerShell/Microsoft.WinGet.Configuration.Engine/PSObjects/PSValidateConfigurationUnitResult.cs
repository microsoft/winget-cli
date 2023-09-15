// -----------------------------------------------------------------------------
// <copyright file="PSValidateConfigurationUnitResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using Microsoft.Management.Configuration;

    /// <summary>
    /// The validate result of a configuration unit.
    /// </summary>
    public class PSValidateConfigurationUnitResult : PSUnitResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSValidateConfigurationUnitResult"/> class.
        /// </summary>
        /// <param name="unitResult">Apply unit result.</param>
        internal PSValidateConfigurationUnitResult(ApplyConfigurationUnitResult unitResult)
            : base(unitResult.Unit, unitResult.ResultInformation)
        {
        }
    }
}
