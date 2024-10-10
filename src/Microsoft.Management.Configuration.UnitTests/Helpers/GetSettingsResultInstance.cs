// -----------------------------------------------------------------------------
// <copyright file="GetSettingsResultInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Implements IGetSettingsResult.
    /// </summary>
    internal sealed partial class GetSettingsResultInstance : IGetSettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetSettingsResultInstance"/> class.
        /// </summary>
        /// <param name="unit">The configuration unit that the result is for.</param>
        public GetSettingsResultInstance(ConfigurationUnit unit)
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
        public ValueSet? Settings { get; internal set; }
    }
}
