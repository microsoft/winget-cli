// -----------------------------------------------------------------------------
// <copyright file="IResourceExportItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Model
{
    using Windows.Foundation.Collections;

    /// <summary>
    /// The interface to a `resource export` command result.
    /// </summary>
    internal interface IResourceExportItem
    {
        /// <summary>
        /// Gets the type of the resource.
        /// </summary>
        public string Type { get; }

        /// <summary>
        /// Gets the name of the resource instance.
        /// </summary>
        public string Name { get; }

        /// <summary>
        /// Gets the settings for this item.
        /// </summary>
        public ValueSet Settings { get; }

        // TODO: Everything else to support Exporter resources (which is everything in the resource instance schema)
        //  Also update DSCv3ConfigurationUnitProcessor to add the new properties to the configuration unit
    }
}
