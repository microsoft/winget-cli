// -----------------------------------------------------------------------------
// <copyright file="ApplyGroupMemberSettingsResultInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    /// <summary>
    /// Implements IApplyGroupMemberSettingsResult.
    /// </summary>
    internal sealed partial class ApplyGroupMemberSettingsResultInstance : IApplyGroupMemberSettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ApplyGroupMemberSettingsResultInstance"/> class.
        /// </summary>
        /// <param name="unit">The unit for this result.</param>
        internal ApplyGroupMemberSettingsResultInstance(ConfigurationUnit unit)
        {
            this.Unit = unit;
        }

        /// <inheritdoc/>
        public bool PreviouslyInDesiredState { get; internal set; }

        /// <inheritdoc/>
        public bool RebootRequired { get; internal set; }

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
        public ConfigurationUnitState State { get; internal set; }

        /// <inheritdoc/>
        public ConfigurationUnit Unit { get; private init; }
    }
}
