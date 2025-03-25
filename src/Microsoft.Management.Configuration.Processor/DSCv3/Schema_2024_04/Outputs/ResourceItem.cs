// -----------------------------------------------------------------------------
// <copyright file="ResourceItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.Collections.Generic;
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Windows.Foundation.Collections;

    /// <summary>
    /// The object type from a single resource item.
    /// </summary>
    internal class ResourceItem : IResourceExportItem
    {
        /// <summary>
        /// Gets or sets the type of the resource.
        /// Should match the regex "^\\w+(\\.\\w+){0,2}\\/\\w+$".
        /// </summary>
        [JsonRequired]
        required public string Type { get; set; }

        /// <summary>
        /// Gets or sets the name of the resource instance.
        /// </summary>
        [JsonRequired]
        required public string Name { get; set; }

        /// <summary>
        /// Gets or sets the properties object.
        /// </summary>
        public JsonObject? Properties { get; set; }

        /// <summary>
        /// Gets or sets the metadata object.
        /// </summary>
        [JsonPropertyName("metadata")]
        public JsonObject? MetadataObject { get; set; }

        /// <summary>
        /// Gets or sets the dependencies.
        /// </summary>
        [JsonPropertyName("dependencies")]
        public List<string> DependenciesList { get; set; } = new List<string>();

        /// <inheritdoc />
        [JsonIgnore]
        public ValueSet Settings
        {
            get
            {
                return this.Properties.ToValueSet();
            }
        }

        /// <inheritdoc />
        [JsonIgnore]
        public ValueSet Metadata
        {
            get
            {
                return this.MetadataObject.ToValueSet();
            }
        }

        /// <inheritdoc />
        [JsonIgnore]
        public IList<string> Dependencies
        {
            get
            {
                return this.DependenciesList;
            }
        }
    }
}
