// -----------------------------------------------------------------------------
// <copyright file="GetAllSettingsResultInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Implements IGetAllSettingsResult.
    /// </summary>
    internal sealed partial class GetAllSettingsResultInstance : IGetAllSettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetAllSettingsResultInstance"/> class.
        /// </summary>
        /// <param name="unit">The configuration unit that the result is for.</param>
        public GetAllSettingsResultInstance(ConfigurationUnit unit)
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
        public IList<ValueSet>? Settings { get; internal set; }
    }
}
