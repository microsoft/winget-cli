// -----------------------------------------------------------------------------
// <copyright file="TestSettingsResultInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Implements ITestSettingsResult.
    /// </summary>
    internal sealed partial class TestSettingsResultInstance : ITestSettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TestSettingsResultInstance"/> class.
        /// </summary>
        /// <param name="unit">The configuration unit that the result is for.</param>
        public TestSettingsResultInstance(ConfigurationUnit unit)
        {
            this.Unit = unit;
        }

        /// <summary>
        /// Gets the configuration unit that the result is for.
        /// </summary>
        public ConfigurationUnit Unit { get; private set; }

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
