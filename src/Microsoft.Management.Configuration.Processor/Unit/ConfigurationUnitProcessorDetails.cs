// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitProcessorDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Unit
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Provides information for a specific configuration unit within the runtime.
    /// </summary>
    internal sealed partial class ConfigurationUnitProcessorDetails : IConfigurationUnitProcessorDetails
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitProcessorDetails"/> class.
        /// </summary>
        public ConfigurationUnitProcessorDetails()
        {
        }

        /// <summary>
        /// Gets the name of the unit of configuration.
        /// </summary>
        required public string UnitType { get; init; }

        /// <summary>
        /// Gets or sets the description of the unit of configuration.
        /// </summary>
        public string? UnitDescription { get; internal set; }

        /// <summary>
        /// Gets or sets the URI of the documentation for the unit of configuration.
        /// </summary>
        public Uri? UnitDocumentationUri { get; internal set; }

        /// <summary>
        /// Gets or sets the URI of the icon for the unit of configuration.
        /// </summary>
        public Uri? UnitIconUri { get; internal set; }

        /// <summary>
        /// Gets or sets the name of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleName { get; internal set; }

        /// <summary>
        /// Gets or sets the type of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleType { get; internal set; }

        /// <summary>
        /// Gets or sets the source of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleSource { get; internal set; }

        /// <summary>
        /// Gets or sets the description of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleDescription { get; internal set; }

        /// <summary>
        /// Gets or sets the URI of the documentation for the module containing the unit of configuration.
        /// </summary>
        public Uri? ModuleDocumentationUri { get; internal set; }

        /// <summary>
        /// Gets or sets the URI for the published module containing the unit of configuration.
        /// </summary>
        public Uri? PublishedModuleUri { get; internal set; }

        /// <summary>
        /// Gets or sets the version of the module containing the unit of configuration.
        /// </summary>
        public string? Version { get; internal set; }

        /// <summary>
        /// Gets or sets the publishing date of the module containing the unit of configuration.
        /// </summary>
        public DateTimeOffset PublishedDate { get; internal set; }

        /// <summary>
        /// Gets or sets a value indicating whether the module is already present on the system.
        /// </summary>
        public bool IsLocal { get; internal set; }

        /// <summary>
        /// Gets or sets the author of the module containing the unit of configuration.
        /// </summary>
        public string? Author { get; internal set; }

        /// <summary>
        /// Gets or sets the publisher of the module containing the unit of configuration.
        /// </summary>
        public string? Publisher { get; internal set; }

        /// <summary>
        /// Gets or sets the signing certificate of the module files containing the unit of configuration.
        /// </summary>
        public IReadOnlyList<object>? SigningInformation { get; internal set; }

        /// <summary>
        /// Gets or sets the settings information for the unit of configuration.
        /// </summary>
        public IReadOnlyList<IConfigurationUnitSettingDetails>? Settings { get; internal set; }

        /// <summary>
        /// Gets or sets a value indicating whether the module comes from a public repository.
        /// </summary>
        public bool IsPublic { get; internal set; }
    }
}
