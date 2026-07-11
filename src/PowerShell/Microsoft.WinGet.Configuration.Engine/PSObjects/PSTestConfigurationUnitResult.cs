// -----------------------------------------------------------------------------
// <copyright file="PSTestConfigurationUnitResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.Helpers;

    /// <summary>
    /// Wrapper for TestConfigurationUnitResult.
    /// </summary>
    public class PSTestConfigurationUnitResult : PSUnitResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PSTestConfigurationUnitResult"/> class.
        /// </summary>
        /// <param name="testUnitResult">Test unit result.</param>
        internal PSTestConfigurationUnitResult(TestConfigurationUnitResult testUnitResult)
            : base(testUnitResult.Unit, testUnitResult.ResultInformation)
        {
            this.TestResult = Utilities.ToPSConfigurationTestResult(testUnitResult.TestResult);
        }

        /// <summary>
        /// Gets the test result.
        /// </summary>
        public PSConfigurationTestResult TestResult { get; private init; }
    }
}
