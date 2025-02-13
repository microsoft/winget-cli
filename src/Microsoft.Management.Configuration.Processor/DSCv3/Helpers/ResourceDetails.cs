// -----------------------------------------------------------------------------
// <copyright file="ResourceDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.Unit;

    /// <summary>
    /// Cached data about a resource.
    /// </summary>
    internal class ResourceDetails
    {
        private readonly ConfigurationUnitInternal configurationUnitInternal;

        private object detailsUpdateLock = new object();

        /// <summary>
        /// The current level of detail stored by this object.
        ///
        /// The method of discovery for each of the levels in ConfigurationUnitDetailFlags:
        ///     None: No details, either because the resource was not found or EnsureDetails has not been called.
        ///     Local: `resource list` is used to determine the details. An embedded schema may enable "Load" details level.
        ///            Property information may not be available at this level.
        ///     Catalog: Same as local; there is currently no catalog to query against.
        ///     Download: Same as local; there is currently no catalog to find anything to download.
        ///     Load: `resource schema` is used to get the full schema for the resource.
        ///           This ensures that property information is available.
        /// </summary>
        private ConfigurationUnitDetailFlags currentDetailLevel = ConfigurationUnitDetailFlags.None;

        /// <summary>
        /// Initializes a new instance of the <see cref="ResourceDetails"/> class.
        /// </summary>
        /// <param name="configurationUnitInternal">The internal configuration unit data.</param>
        public ResourceDetails(ConfigurationUnitInternal configurationUnitInternal)
        {
            this.configurationUnitInternal = configurationUnitInternal;
        }

        /// <summary>
        /// Gets a value indicating whether this resource exists.
        /// Will be false until EnsureDetails is called and the resource is found.
        /// </summary>
        public bool Exists
        {
            get
            {
                return this.currentDetailLevel != ConfigurationUnitDetailFlags.None;
            }
        }

        /// <summary>
        /// Ensures that the given detail level is present.
        /// </summary>
        /// <param name="processorSettings">The processor settings to use when getting details.</param>
        /// <param name="detailFlags">The detail level flags.</param>
        public void EnsureDetails(ProcessorSettings processorSettings, ConfigurationUnitDetailFlags detailFlags)
        {
            if (this.DetailsNeededFor(detailFlags, ConfigurationUnitDetailFlags.Local))
            {
                // If we can't get local details, then exit until we have more options.
                if (!this.GetLocalDetails(processorSettings))
                {
                    return;
                }
            }

            if (this.DetailsNeededFor(detailFlags, ConfigurationUnitDetailFlags.Load))
            {
                this.GetLoadDetails(processorSettings);
            }
        }

        /// <summary>
        /// Gets a ConfigurationUnitProcessorDetails populated with all available data.
        /// </summary>
        /// <returns>A ConfigurationUnitProcessorDetails populated with all available data.</returns>
        public ConfigurationUnitProcessorDetails GetConfigurationUnitProcessorDetails()
        {
            ConfigurationUnitProcessorDetails result = new ConfigurationUnitProcessorDetails() { UnitType = this.configurationUnitInternal.QualifiedName };
            result.UnitDescription = "This is a test!";
            return result;
        }

        private bool DetailsNeededFor(ConfigurationUnitDetailFlags detailFlags, ConfigurationUnitDetailFlags targetLevel)
        {
            if (!detailFlags.HasFlag(targetLevel))
            {
                return false;
            }

            lock (this.detailsUpdateLock)
            {
                return !this.currentDetailLevel.HasFlag(targetLevel);
            }
        }
    }
}
