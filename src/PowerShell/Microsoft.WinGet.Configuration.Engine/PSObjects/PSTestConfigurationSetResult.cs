// -----------------------------------------------------------------------------
// <copyright file="PSTestConfigurationSetResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Helpers;

    /// <summary>
    /// Wrapper for TestConfigurationSetResult.
    /// </summary>
    public class PSTestConfigurationSetResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSTestConfigurationSetResult"/> class.
        /// </summary>
        /// <param name="testSetResult">Test set result.</param>
        internal PSTestConfigurationSetResult(TestConfigurationSetResult testSetResult)
        {
            this.TestResult = Utilities.ToPSConfigurationTestResult(testSetResult.TestResult);

            var unitResults = new List<PSTestConfigurationUnitResult>();
            foreach (var unitResult in testSetResult.UnitResults)
            {
                unitResults.Add(new PSTestConfigurationUnitResult(unitResult));
            }

            this.UnitResults = unitResults;
        }

        /// <summary>
        /// Gets the test result.
        /// </summary>
        public PSConfigurationTestResult TestResult { get; private init; }

        /// <summary>
        /// Gets the results of the units.
        /// </summary>
        public IReadOnlyList<PSTestConfigurationUnitResult> UnitResults { get; private init; }
    }
}
