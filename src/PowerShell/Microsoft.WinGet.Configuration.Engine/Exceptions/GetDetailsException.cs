// -----------------------------------------------------------------------------
// <copyright file="GetDetailsException.cs" company="Microsoft Corporation">
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
    /// Exception thrown while getting details.
    /// </summary>
    public class GetDetailsException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetDetailsException"/> class.
        /// </summary>
        /// <param name="unitResults">Unit results.</param>
        internal GetDetailsException(IReadOnlyList<GetConfigurationUnitDetailsResult>? unitResults = null)
            : base(Resources.ConfigurationFailedToGetDetails)
        {
            var results = new List<PSGetConfigurationDetailsResult>();

            if (unitResults != null)
            {
                foreach (var result in unitResults)
                {
                    results.Add(new PSGetConfigurationDetailsResult(result));
                }
            }

            this.UnitDetailsResults = results;
        }

        /// <summary>
        /// Gets the unit details result.
        /// </summary>
        public IReadOnlyList<PSGetConfigurationDetailsResult> UnitDetailsResults { get; private init; }
    }
}
