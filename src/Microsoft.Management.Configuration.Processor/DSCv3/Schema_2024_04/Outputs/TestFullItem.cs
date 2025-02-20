// -----------------------------------------------------------------------------
// <copyright file="TestFullItem.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Outputs
{
    using System.IO;
    using System.Text.Json;
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Metadata;

    /// <summary>
    /// The full form of the test output.
    /// When the retrieved instance is for group resource, adapter resource, or nested inside a group or adapter resource, DSC returns a full test result, which also includes the resource type and instance name.
    /// </summary>
    internal class TestFullItem : IResourceTestItem
    {
        private const string InDesiredStatePropertyName = "inDesiredState";

        /// <summary>
        /// Initializes a new instance of the <see cref="TestFullItem"/> class.
        /// </summary>
        public TestFullItem()
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
        public TestSimpleItem? SimpleResult { get; set; }

        /// <summary>
        /// Gets or sets the full results.
        /// </summary>
        [JsonIgnore]
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Opening square brackets should be spaced correctly", Justification = "Pending SC 1.2 release")]
        public TestFullItem[]? FullResults { get; set; }

        /// <inheritdoc />
        public bool InDesiredState
        {
            get
            {
                if (this.SimpleResult != null)
                {
                    return this.SimpleResult.InDesiredState;
                }
                else if (this.FullResults != null)
                {
                    bool result = true;

                    foreach (var item in this.FullResults)
                    {
                        result = result && item.InDesiredState;
                    }

                    return result;
                }
                else
                {
                    throw new System.InvalidOperationException("Test result has not been initialized.");
                }
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="TestFullItem"/> class.
        /// </summary>
        /// <param name="document">The document to construct from.</param>
        /// <param name="options">The options to use.</param>
        /// <returns>The item created.</returns>
        public static TestFullItem CreateFrom(JsonDocument document, JsonSerializerOptions options)
        {
            if (document.RootElement.TryGetProperty(InDesiredStatePropertyName, out JsonElement jsonElement))
            {
                return new () { SimpleResult = JsonSerializer.Deserialize<TestSimpleItem>(document, options) };
            }
            else
            {
                TestFullItem? result = JsonSerializer.Deserialize<TestFullItem>(document, options);

                if (result == null)
                {
                    throw new InvalidDataException("Unable to deserialize full test result.");
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
                throw new System.InvalidOperationException("Test result has not been initialized.");
            }

            if (this.Result is JsonObject jsonObject)
            {
                this.SimpleResult = JsonSerializer.Deserialize<TestSimpleItem>(this.Result, options);
            }
            else
            {
                this.FullResults = JsonSerializer.Deserialize<TestFullItem[]>(this.Result, options);

                if (this.FullResults == null)
                {
                    throw new InvalidDataException("Unable to deserialize full test results.");
                }

                foreach (TestFullItem result in this.FullResults)
                {
                    result.ProcessResult(options);
                }
            }
        }
    }
}
