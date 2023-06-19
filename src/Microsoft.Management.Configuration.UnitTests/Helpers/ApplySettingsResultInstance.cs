// -----------------------------------------------------------------------------
// <copyright file="TestApplySettingsResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Implements IApplySettingsResult.
    /// </summary>
    internal sealed class ApplySettingsResultInstance : IApplySettingsResult
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
        public bool RebootRequired { get; internal set; }
    }
}
