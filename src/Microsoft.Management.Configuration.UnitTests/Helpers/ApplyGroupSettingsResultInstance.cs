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
    internal sealed class ApplyGroupSettingsResultInstance : IApplyGroupSettingsResult
    {
        /// <inheritdoc/>
        public object? Group { get; internal set; }

        /// <inheritdoc/>
        public bool RebootRequired { get; internal set; }

        /// <inheritdoc/>
        public IConfigurationUnitResultInformation? ResultInformation { get; internal set; }

        /// <inheritdoc/>
        public IList<IApplyGroupMemberSettingsResult>? UnitResults { get; internal set; }
    }
}
