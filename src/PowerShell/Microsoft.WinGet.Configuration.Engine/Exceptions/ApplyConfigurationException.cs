// -----------------------------------------------------------------------------
// <copyright file="ApplyConfigurationException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Exceptions
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Exception thrown when there's an error when configuration is applied.
    /// </summary>
    public class ApplyConfigurationException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ApplyConfigurationException"/> class.
        /// </summary>
        /// <param name="applyResult">Apply Result.</param>
        internal ApplyConfigurationException(ApplyConfigurationSetResult applyResult)
            : base(Resources.ConfigurationFailedToApply)
        {
            this.HResult = applyResult.ResultCode?.HResult ?? ErrorCodes.WingetConfigErrorSetApplyFailed;

            var results = new List<PSApplyConfigurationUnitResult>();
            foreach (var unitResult in applyResult.UnitResults)
            {
                results.Add(new PSApplyConfigurationUnitResult(unitResult));
            }

            this.UnitResults = results;
        }

        /// <summary>
        /// Gets the result of the units.
        /// </summary>
        public IReadOnlyList<PSApplyConfigurationUnitResult> UnitResults { get; private init; }
    }
}
