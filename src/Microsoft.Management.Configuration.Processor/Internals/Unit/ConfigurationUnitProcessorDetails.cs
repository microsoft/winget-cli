// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitProcessorDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Internals.Unit
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration;
    using Windows.Security.Cryptography.Certificates;

    /// <summary>
    /// Provides information for a specific configuration unit within the runtime.
    /// </summary>
    internal sealed class ConfigurationUnitProcessorDetails : IConfigurationUnitProcessorDetails
    {
        /// <summary>
        /// Gets the name of the unit of configuration.
        /// </summary>
        public string? UnitName { get; }

        /// <summary>
        /// Gets the description of the unit of configuration.
        /// </summary>
        public string? UnitDescription { get; }

        /// <summary>
        /// Gets the URI of the documentation for the unit of configuration.
        /// </summary>
        public Uri? UnitDocumentationUri { get; }

        /// <summary>
        /// Gets the URI of the icon for the unit of configuration.
        /// </summary>
        public Uri? UnitIconUri { get; }

        /// <summary>
        /// Gets the name of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleName { get; }

        /// <summary>
        /// Gets the type of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleType { get; }

        /// <summary>
        /// Gets the source of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleSource { get; }

        /// <summary>
        /// Gets the description of the module containing the unit of configuration.
        /// </summary>
        public string? ModuleDescription { get; }

        /// <summary>
        /// Gets the URI of the documentation for the module containing the unit of configuration.
        /// </summary>
        public Uri? ModuleDocumentationUri { get; }

        /// <summary>
        /// Gets the URI for the published module containing the unit of configuration.
        /// </summary>
        public Uri? PublishedModuleUri { get; }

        /// <summary>
        /// Gets the version of the module containing the unit of configuration.
        /// </summary>
        public string? Version { get; }

        /// <summary>
        /// Gets the publishing date of the module containing the unit of configuration.
        /// </summary>
        public DateTimeOffset PublishedDate { get; }

        /// <summary>
        /// Gets a value indicating whether the module is already present on the system.
        /// </summary>
        public bool IsLocal { get; }

        /// <summary>
        /// Gets the author of the module containing the unit of configuration.
        /// </summary>
        public string? Author { get; }

        /// <summary>
        /// Gets the publisher of the module containing the unit of configuration.
        /// </summary>
        public string? Publisher { get; }

        /// <summary>
        /// Gets the signing certificate chain of the module containing the unit of configuration.
        /// </summary>
        public CertificateChain? SigningCertificateChain { get; }

        /// <summary>
        /// Gets the settings information for the unit of configuration.
        /// </summary>
        public IReadOnlyList<IConfigurationUnitSettingDetails>? Settings { get; }
    }
}
