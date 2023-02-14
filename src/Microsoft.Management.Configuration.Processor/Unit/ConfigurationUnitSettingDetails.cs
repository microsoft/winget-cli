// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitSettingDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using Microsoft.Management.Configuration;
    using Windows.Foundation;

    /// <summary>
    /// Provides information for a specific configuration unit setting.
    /// </summary>
    internal sealed class ConfigurationUnitSettingDetails : IConfigurationUnitSettingDetails
    {
        /// <summary>
        /// Gets the name of the setting.
        /// </summary>
        public string? Name { get; }

        /// <summary>
        /// Gets the description of the setting.
        /// </summary>
        public string? Description { get; }

        /// <summary>
        /// Gets a value indicating whether the setting is a key. This is used to determine if different settings are in conflict.
        /// </summary>
        public bool IsKey { get; }

        /// <summary>
        /// Gets a value indicating whether a non-empty value for the setting is required.
        /// </summary>
        public bool IsRequired { get; }

        /// <summary>
        /// Gets a value indicating whether the setting should be serialized in order to be applied on another system.
        /// </summary>
        public bool IsInformational { get; }

        /// <summary>
        /// Gets the data type for the value of this setting.
        /// </summary>
        public PropertyType Type { get; }

        /// <summary>
        /// Gets the semantics to be used for this setting. The goal is to enable richer conflict detection and authoring
        /// scenarios by having a deeper understanding of this value than "String".
        /// </summary>
        public string? Semantics { get; }
    }
}
