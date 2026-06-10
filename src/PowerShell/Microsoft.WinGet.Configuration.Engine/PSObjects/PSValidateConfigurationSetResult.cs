// -----------------------------------------------------------------------------
// <copyright file="PSValidateConfigurationSetResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;

    /// <summary>
    /// Wrapper for ApplyConfigurationSetResult for validate.
    /// </summary>
    public class PSValidateConfigurationSetResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSValidateConfigurationSetResult"/> class.
        /// </summary>
        /// <param name="applySetResult">Apply set result.</param>
        internal PSValidateConfigurationSetResult(ApplyConfigurationSetResult applySetResult)
        {
            this.ResultCode = applySetResult.ResultCode?.HResult ?? ErrorCodes.S_OK;

            var unitResults = new List<PSValidateConfigurationUnitResult>();
            foreach (var unitResult in applySetResult.UnitResults)
            {
                unitResults.Add(new PSValidateConfigurationUnitResult(unitResult));
            }

            this.UnitResults = unitResults;
        }

        /// <summary>
        /// Gets the result code.
        /// </summary>
        public int ResultCode { get; private init; }

        /// <summary>
        /// Gets the results of the units.
        /// </summary>
        public IReadOnlyList<PSValidateConfigurationUnitResult> UnitResults { get; private init; }
    }
}
