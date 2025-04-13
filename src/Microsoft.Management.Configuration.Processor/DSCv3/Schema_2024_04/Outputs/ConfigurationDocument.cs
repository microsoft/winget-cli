// -----------------------------------------------------------------------------
// <copyright file="ConfigurationDocument.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text.Json;
    using System.Text.Json.Serialization;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// A configuration document.
    /// </summary>
    internal class ConfigurationDocument
    {
        /// <summary>
        /// Gets or sets the list of resources in the document.
        /// </summary>
        public List<ResourceItem> Resources { get; set; } = new List<ResourceItem>();

        /// <summary>
        /// Gets the list of resources as the interface version.
        /// </summary>
        [JsonIgnore]
        public IList<IResourceExportItem> InterfaceResources
        {
            get
            {
                return new List<IResourceExportItem>(this.Resources.AsEnumerable<IResourceExportItem>());
            }
        }

        /// <summary>
        /// Initializes a new instance of the ConfigurationDocument class.
        /// </summary>
        /// <param name="document">The document to construct from.</param>
        /// <param name="options">The options to use.</param>
        /// <returns>The item created.</returns>
        public static ConfigurationDocument CreateFrom(JsonDocument document, JsonSerializerOptions options)
        {
            ConfigurationDocument? result = JsonSerializer.Deserialize<ConfigurationDocument>(document, options);

            if (result == null)
            {
                throw new InvalidDataException("Unable to deserialize ConfigurationDocument.");
            }

            return result;
        }
    }
}
