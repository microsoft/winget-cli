// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitResultInformation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using System;
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Implements IConfigurationUnitResultInformation.
    /// </summary>
    internal sealed partial class ConfigurationUnitResultInformation : IConfigurationUnitResultInformation
    {
        /// <inheritdoc/>
        public string? Description { get; internal set; }

        /// <inheritdoc/>
        public string? Details { get; internal set; }

        /// <inheritdoc/>
        public Exception? ResultCode { get; internal set; }

        /// <inheritdoc/>
        public ConfigurationUnitResultSource ResultSource { get; internal set; } = ConfigurationUnitResultSource.None;
    }
}
