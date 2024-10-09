// -----------------------------------------------------------------------------
// <copyright file="ApplySettingsResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Implements IApplySettingsResult.
    /// </summary>
    internal sealed partial class ApplySettingsResult : IApplySettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ApplySettingsResult"/> class.
        /// </summary>
        /// <param name="unit">The configuration unit that the result is for.</param>
        public ApplySettingsResult(ConfigurationUnit unit)
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
        public bool RebootRequired { get; internal set; }
    }
}
