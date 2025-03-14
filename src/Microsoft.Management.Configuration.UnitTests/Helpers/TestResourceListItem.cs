// -----------------------------------------------------------------------------
// <copyright file="TestResourceListItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// Implements IResourceListItem for tests.
    /// </summary>
    internal class TestResourceListItem : IResourceListItem
    {
        /// <summary>
        /// Gets or sets the type.
        /// </summary>
        required public string Type { get; set; }

        /// <summary>
        /// Gets or sets the kind.
        /// </summary>
        public ResourceKind Kind { get; set; }

        /// <summary>
        /// Gets or sets the version.
        /// </summary>
        public string? Version { get; set; }

        /// <summary>
        /// Gets or sets the description.
        /// </summary>
        public string? Description { get; set; }

        /// <summary>
        /// Gets or sets the directory.
        /// </summary>
        public string? Directory { get; set; }

        /// <summary>
        /// Gets or sets the author.
        /// </summary>
        public string? Author { get; set; }
    }
}
