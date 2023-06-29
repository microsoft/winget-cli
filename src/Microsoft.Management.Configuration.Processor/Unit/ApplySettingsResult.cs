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
    internal sealed class ApplySettingsResult : IApplySettingsResult
    {
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
