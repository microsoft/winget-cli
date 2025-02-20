// -----------------------------------------------------------------------------
// <copyright file="IResourceListItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Model
{
    /// <summary>
    /// The interface to a single JSON line output by the `resource list` command.
    /// </summary>
    internal interface IResourceListItem
    {
        /// <summary>
        /// Gets the type of the resource.
        /// Should match the regex "^\\w+(\\.\\w+){0,2}\\/\\w+$".
        /// </summary>
        public string Type { get; }

        /// <summary>
        /// Gets the kind of the resource.
        /// </summary>
        public ResourceKind Kind { get; }

        /// <summary>
        /// Gets the version of the resource.
        /// This is a semver version.
        /// </summary>
        public string? Version { get; }

        /// <summary>
        /// Gets the description of the resource.
        /// </summary>
        public string? Description { get; }

        /// <summary>
        /// Gets the path to the directory containing the resource.
        /// </summary>
        public string? Directory { get; }

        /// <summary>
        /// Gets the author of the resource.
        /// </summary>
        public string? Author { get; }
    }
}
