// -----------------------------------------------------------------------------
// <copyright file="ResourceListItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// The object type from a single JSON line output by the `resource list` command.
    /// </summary>
    internal class ResourceListItem : IResourceListItem
    {
        /// <summary>
        /// Gets or sets the type of the resource.
        /// Should match the regex "^\\w+(\\.\\w+){0,2}\\/\\w+$".
        /// </summary>
        [JsonRequired]
        required public string Type { get; set; }

        /// <summary>
        /// Gets or sets the kind of the resource.
        /// </summary>
        public Definitions.ResourceKind Kind { get; set; } = Definitions.ResourceKind.Unknown;

        /// <inheritdoc />
        [JsonIgnore]
        Model.ResourceKind IResourceListItem.Kind => this.Kind switch
        {
            Definitions.ResourceKind.Unknown => Model.ResourceKind.Unknown,
            Definitions.ResourceKind.Resource => Model.ResourceKind.Resource,
            Definitions.ResourceKind.Adapter => Model.ResourceKind.Adapter,
            Definitions.ResourceKind.Group => Model.ResourceKind.Group,
            Definitions.ResourceKind.Import => Model.ResourceKind.Import,
            _ => throw new System.IO.InvalidDataException($"Unknown ResourceKind: {this.Kind}")
        };

        /// <summary>
        /// Gets or sets the version of the resource.
        /// This is a semver version.
        /// </summary>
        public string? Version { get; set; }

        /// <summary>
        /// Gets or sets the capabilities of the resource.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
        public ResourceCapability[] Capabilities { get; set; } = [];

        /// <summary>
        /// Gets or sets the description of the resource.
        /// </summary>
        public string? Description { get; set; }

        /// <summary>
        /// Gets or sets the path to the resource definition file.
        /// </summary>
        public string? Path { get; set; }

        /// <summary>
        /// Gets or sets the path to the directory containing the resource.
        /// </summary>
        public string? Directory { get; set; }

        /// <summary>
        /// Gets or sets a value that indicates implementation details of the resource.
        /// </summary>
        public JsonObject? ImplementedAs { get; set; }

        /// <summary>
        /// Gets or sets the author of the resource.
        /// </summary>
        public string? Author { get; set; }

        /// <summary>
        /// Gets or sets the names of the properties of the resource.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
        public string[] Properties { get; set; } = [];

        /// <summary>
        /// Gets or sets the adapter required by the resource.
        /// </summary>
        public string? RequireAdapter { get; set; }

        /// <summary>
        /// Gets or sets the resource definition manifest.
        /// </summary>
        public JsonObject? Manifest { get; set; }
    }
}
