// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationUnitProcessorDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;
    using Windows.Security.Cryptography.Certificates;

    /// <summary>
    /// A test implementation of IConfigurationProcessorFactory.
    /// </summary>
    internal partial class TestConfigurationUnitProcessorDetails : IConfigurationUnitProcessorDetails
    {
        private ConfigurationUnit unit;
        private ConfigurationUnitDetailFlags detailFlags;

        /// <summary>
        /// Initializes a new instance of the <see cref="TestConfigurationUnitProcessorDetails"/> class.
        /// </summary>
        /// <param name="unit">The unit.</param>
        /// <param name="detailFlags">The flags of the details.</param>
        internal TestConfigurationUnitProcessorDetails(ConfigurationUnit unit, ConfigurationUnitDetailFlags detailFlags)
        {
            this.unit = unit;
            this.detailFlags = detailFlags;
        }

#pragma warning disable SA1600 // Elements should be documented
        public string? Author { get; internal set; }

        public bool IsLocal { get; internal set; }

        public string? ModuleDescription { get; internal set; }

        public Uri? ModuleDocumentationUri { get; internal set; }

        public string? ModuleName { get; internal set; }

        public string? ModuleSource { get; internal set; }

        public string? ModuleType { get; internal set; }

        public DateTimeOffset PublishedDate { get; internal set; }

        public Uri? PublishedModuleUri { get; internal set; }

        public string? Publisher { get; internal set; }

        public IReadOnlyList<IConfigurationUnitSettingDetails>? Settings { get; internal set; }

        public IReadOnlyList<object>? SigningInformation { get; internal set; }

        public string? UnitDescription { get; internal set; }

        public Uri? UnitDocumentationUri { get; internal set; }

        public Uri? UnitIconUri { get; internal set; }

        public string? UnitType { get; internal set; }

        public string? Version { get; internal set; }

        public bool IsPublic { get; internal set; }
#pragma warning restore SA1600 // Elements should be documented
    }
}
