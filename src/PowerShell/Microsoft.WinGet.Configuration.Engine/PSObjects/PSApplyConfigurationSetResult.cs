// -----------------------------------------------------------------------------
// <copyright file="PSApplyConfigurationSetResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;

    /// <summary>
    /// Wrapper for ApplyConfigurationSetResult.
    /// </summary>
    public class PSApplyConfigurationSetResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSApplyConfigurationSetResult"/> class.
        /// </summary>
        /// <param name="applySetResult">Apply set result.</param>
        internal PSApplyConfigurationSetResult(ApplyConfigurationSetResult applySetResult)
        {
            this.ResultCode = applySetResult.ResultCode?.HResult ?? ErrorCodes.S_OK;

            var unitResults = new List<PSApplyConfigurationUnitResult>();
            foreach (var unitResult in applySetResult.UnitResults)
            {
                unitResults.Add(new PSApplyConfigurationUnitResult(unitResult));
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
        public IReadOnlyList<PSApplyConfigurationUnitResult> UnitResults { get; private init; }
    }
}
