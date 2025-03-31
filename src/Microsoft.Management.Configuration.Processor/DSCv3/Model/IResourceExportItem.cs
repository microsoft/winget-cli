// -----------------------------------------------------------------------------
// <copyright file="IResourceExportItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Model
{
    using System.Collections.Generic;
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

        /// <summary>
        /// Gets the metadata for this item.
        /// </summary>
        public ValueSet Metadata { get; }

        /// <summary>
        /// Gets the dependencies for this item.
        /// </summary>
        public IList<string> Dependencies { get; }
    }
}
