// -----------------------------------------------------------------------------
// <copyright file="GetAllSettingsResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Implements IGetAllSettingsResult.
    /// </summary>
    internal partial class GetAllSettingsResult : IGetAllSettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GetAllSettingsResult"/> class.
        /// </summary>
        /// <param name="unit">The configuration unit that the result is for.</param>
        public GetAllSettingsResult(ConfigurationUnit unit)
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
        public ConfigurationUnitResultInformation InternalResult { get; } = new ConfigurationUnitResultInformation();

        /// <inheritdoc/>
        public IList<ValueSet>? Settings { get; internal set; }
    }
}
