// -----------------------------------------------------------------------------
// <copyright file="ApplyGroupSettingsResultInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Collections.Generic;

    /// <summary>
    /// Implements IApplyGroupSettingsResult.
    /// </summary>
    internal sealed partial class ApplyGroupSettingsResultInstance : IApplyGroupSettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ApplyGroupSettingsResultInstance"/> class.
        /// </summary>
        /// <param name="group">The group for this result.</param>
        internal ApplyGroupSettingsResultInstance(object? group)
        {
            this.Group = group;
        }

        /// <inheritdoc/>
        public object? Group { get; private init; }

        /// <inheritdoc/>
        public bool RebootRequired { get; internal set; }

        /// <inheritdoc/>
        public IConfigurationUnitResultInformation? ResultInformation
        {
            get { return this.InternalResult; }
        }

        /// <summary>
        /// Gets the implementation object for ResultInformation.
        /// </summary>
        public TestConfigurationUnitResultInformation InternalResult { get; } = new TestConfigurationUnitResultInformation();

        /// <inheritdoc/>
        public IList<IApplyGroupMemberSettingsResult>? UnitResults { get; internal set; }
    }
}
