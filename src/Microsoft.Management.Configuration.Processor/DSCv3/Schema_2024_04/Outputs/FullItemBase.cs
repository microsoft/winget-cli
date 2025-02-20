// -----------------------------------------------------------------------------
// <copyright file="FullItemBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.IO;
    using System.Text.Json;
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;
    using Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Metadata;

    /// <summary>
    /// The base implementation of the full form output.
    /// When the retrieved instance is for group resource, adapter resource, or nested inside a group or adapter resource, DSC returns a full result, which also includes the resource type and instance name.
    /// </summary>
    /// <typeparam name="TSimple">The simple item type.</typeparam>
    /// <typeparam name="TFull">The full item type.</typeparam>
    internal class FullItemBase<TSimple, TFull>
        where TFull : FullItemBase<TSimple, TFull>, new()
    {
        private const string NameProperty = "name";

        /// <summary>
        /// Initializes a new instance of the <see cref="FullItemBase{TSimple,TFull}"/> class.
        /// </summary>
        public FullItemBase()
        {
        }

        /// <summary>
        /// Gets or sets the metadata for this test result.
        /// </summary>
        [JsonRequired]
        public ResourceInstanceResult? Metadata { get; set; }

        /// <summary>
        /// Gets or sets the name for this test result.
        /// </summary>
        [JsonRequired]
        public string? Name { get; set; }

        /// <summary>
        /// Gets or sets the type for the resource.
        /// </summary>
        [JsonRequired]
        public string? Type { get; set; }

        /// <summary>
        /// Gets or sets the result of the test.
        /// </summary>
        [JsonRequired]
        public JsonNode? Result { get; set; }

        /// <summary>
        /// Gets or sets the simple result.
        /// </summary>
        [JsonIgnore]
        public TSimple? SimpleResult { get; set; }

        /// <summary>
        /// Gets or sets the full results.
        /// </summary>
        [JsonIgnore]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Opening square brackets should be spaced correctly", Justification = "Pending SC 1.2 release")]
        protected TFull[]? FullResults { get; set; }

        /// <summary>
        /// Initializes a new instance of the TFull class.
        /// </summary>
        /// <param name="document">The document to construct from.</param>
        /// <param name="options">The options to use.</param>
        /// <returns>The item created.</returns>
        public static TFull CreateFrom(JsonDocument document, JsonSerializerOptions options)
        {
            if (!document.RootElement.TryGetProperty(NameProperty, out JsonElement jsonElement))
            {
                return new () { SimpleResult = JsonSerializer.Deserialize<TSimple>(document, options) };
            }
            else
            {
                TFull? result = JsonSerializer.Deserialize<TFull>(document, options);

                if (result == null)
                {
                    throw new InvalidDataException("Unable to deserialize full result.");
                }

                result.ProcessResult(options);
                return result;
            }
        }

        /// <summary>
        /// Converts the Result property into the appropriate simple or full results.
        /// </summary>
        /// <param name="options">The options to use.</param>
        public void ProcessResult(JsonSerializerOptions options)
        {
            if (this.Result == null)
            {
                throw new System.InvalidOperationException("JSON result has not been initialized.");
            }

            if (this.Result is JsonObject jsonObject)
            {
                this.SimpleResult = JsonSerializer.Deserialize<TSimple>(this.Result, options);
            }
            else
            {
                this.FullResults = JsonSerializer.Deserialize<TFull[]>(this.Result, options);

                if (this.FullResults == null)
                {
                    throw new InvalidDataException("Unable to deserialize full results.");
                }

                foreach (TFull result in this.FullResults)
                {
                    result.ProcessResult(options);
                }
            }
        }
    }
}
