// -----------------------------------------------------------------------------
// <copyright file="TestTestSettingsResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Implements ITestSettingsResult.
    /// </summary>
    internal sealed class TestSettingsResultInstance : ITestSettingsResult
    {
        /// <inheritdoc/>
        public IConfigurationUnitResultInformation ResultInformation
        {
            get { return this.InternalResult; }
        }

        /// <summary>
        /// Gets the implementation object for ResultInformation.
        /// </summary>
        public TestConfigurationUnitResultInformation InternalResult { get; } = new TestConfigurationUnitResultInformation();

        /// <inheritdoc/>
        public ConfigurationTestResult TestResult { get; internal set; }
    }
}
