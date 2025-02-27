// -----------------------------------------------------------------------------
// <copyright file="IResourceGetItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Model
{
    using Windows.Foundation.Collections;

    /// <summary>
    /// The interface to a `resource get` command result.
    /// </summary>
    internal interface IResourceGetItem
    {
        /// <summary>
        /// Gets the settings for this item.
        /// </summary>
        public ValueSet Settings { get; }
    }
}
