// -----------------------------------------------------------------------------
// <copyright file="ResourceDetails.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.Unit;

    /// <summary>
    /// Cached data about a resource.
    /// </summary>
    internal class ResourceDetails
    {
        private readonly string resourceTypeName;

        private object detailsUpdateLock = new object();
        private IResourceListItem? resourceListItem = null;

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
        /// <param name="resourceTypeName">The resource type name.</param>
        public ResourceDetails(string resourceTypeName)
        {
            this.resourceTypeName = resourceTypeName;
        }

        /// <summary>
        /// Gets a value indicating whether this resource exists.
        /// Will be false until EnsureDetails is called and the resource is found.
        /// </summary>
        public bool Exists
        {
            get
            {
                lock (this.detailsUpdateLock)
                {
                    return this.currentDetailLevel != ConfigurationUnitDetailFlags.None;
                }
            }
        }

        /// <summary>
        /// Gets the path of the resource.
        /// </summary>
        public string? Path
        {
            get
            {
                lock (this.detailsUpdateLock)
                {
                    return this.resourceListItem?.Path;
                }
            }
        }

        /// <summary>
        /// Sets the resource list item directly to avoid duplicate "dsc resource list" calls.
        /// </summary>
        /// <param name="item">The resource list item.</param>
        public void SetResourceListItem(IResourceListItem item)
        {
            lock (this.detailsUpdateLock)
            {
                if (this.resourceListItem != null)
                {
                    throw new InvalidOperationException("Resource list item is already set");
                }

                this.resourceListItem = item;
                this.currentDetailLevel |= ConfigurationUnitDetailFlags.Local;
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
        public ConfigurationUnitProcessorDetails? GetConfigurationUnitProcessorDetails()
        {
            if (!this.Exists)
            {
                return null;
            }

            ConfigurationUnitProcessorDetails result = new ConfigurationUnitProcessorDetails() { UnitType = this.resourceTypeName };

            lock (this.detailsUpdateLock)
            {
                if (this.resourceListItem != null)
                {
                    result.UnitType = this.resourceListItem.Type;
                    result.IsGroup = IsGroup(this.resourceListItem.Kind);
                    result.Version = this.resourceListItem.Version;
                    result.UnitDescription = this.resourceListItem.Description;
                    result.Author = this.resourceListItem.Author;
                    result.Path = this.resourceListItem.Path;

                    result.IsLocal = true;
                }
            }

            return result;
        }

        private static bool IsGroup(ResourceKind kind) => kind switch
        {
            ResourceKind.Adapter => true,
            ResourceKind.Group => true,
            _ => false,
        };

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

        private bool GetLocalDetails(ProcessorSettings processorSettings)
        {
            IResourceListItem? resourceListItem = processorSettings.DSCv3.GetResourceByType(this.resourceTypeName, null);

            if (resourceListItem != null)
            {
                // TODO: Attempt to extract embedded schema to avoid the need for Load.
                lock (this.detailsUpdateLock)
                {
                    if (!this.currentDetailLevel.HasFlag(ConfigurationUnitDetailFlags.Local))
                    {
                        this.resourceListItem = resourceListItem;
                        this.currentDetailLevel |= ConfigurationUnitDetailFlags.Local;
                    }
                }

                return true;
            }
            else
            {
                return false;
            }
        }

        private void GetLoadDetails(ProcessorSettings processorSettings)
        {
            throw new NotImplementedException();
        }
    }
}
