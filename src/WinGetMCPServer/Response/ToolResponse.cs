// -----------------------------------------------------------------------------
// <copyright file="ToolResponse.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    using Microsoft.WinGet.SharedLib.PolicySettings;
    using ModelContextProtocol.Protocol;
    using System.Text.Json;
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;
    using WinGetMCPServer.Exceptions;

    /// <summary>
    /// Contains reusable responses for tools.
    /// </summary>
    internal static class ToolResponse
    {
        /// <summary>
        /// The serializer options are created once and shared; System.Text.Json caches the metadata it
        /// builds per options instance, so creating a new instance for each response would discard that
        /// cache every time.
        /// </summary>
        private static readonly JsonSerializerOptions DefaultJsonOptions = CreateDefaultJsonOptions();

        /// <summary>
        /// Checks whether the server is disabled by group policy.
        /// </summary>
        public static void CheckGroupPolicy()
        {
            if (!GroupPolicy.GetInstance().IsEnabled(Policy.McpServer))
            {
                throw new ToolResponseException(new CallToolResult()
                {
                    IsError = true,
                    Content = [new TextContentBlock() { Text = "The Windows Package Manager MCP server is disabled by group policy." }]
                });
            }
        }

        /// <summary>
        /// Constructs a response from an object.
        /// </summary>
        /// <param name="value">The object to return in the response.</param>
        /// <param name="isError">Whether or not the response is an error.</param>
        /// <returns>The response.</returns>
        public static CallToolResult FromObject(object value, bool isError = false)
        {
            return FromObject(value, isError, DefaultJsonOptions);
        }

        /// <summary>
        /// Constructs a response from an object.
        /// </summary>
        /// <param name="value">The object to return in the response.</param>
        /// <param name="isError">Whether or not the response is an error.</param>
        /// <param name="jsonSerializerOptions">The JSON serializer options for serializing the object.</param>
        /// <returns>The response.</returns>
        public static CallToolResult FromObject(object value, bool isError, JsonSerializerOptions jsonSerializerOptions)
        {
            return Create(value, isError, jsonSerializerOptions, collectionPropertyName: null);
        }

        /// <summary>
        /// Constructs a response from a collection.
        /// </summary>
        /// <param name="value">The collection to return in the response.</param>
        /// <param name="collectionPropertyName">
        /// The property name to expose the collection under in the structured content. Structured
        /// content must be a JSON object, so a collection has to be wrapped in one.
        /// </param>
        /// <param name="isError">Whether or not the response is an error.</param>
        /// <returns>The response.</returns>
        public static CallToolResult FromCollection(object value, string collectionPropertyName, bool isError = false)
        {
            return Create(value, isError, DefaultJsonOptions, collectionPropertyName);
        }

        /// <summary>
        /// Gets the default serialization options.
        /// </summary>
        /// <returns>The default serialization options.</returns>
        public static JsonSerializerOptions GetDefaultJsonOptions()
        {
            return DefaultJsonOptions;
        }

        private static CallToolResult Create(object value, bool isError, JsonSerializerOptions jsonSerializerOptions, string? collectionPropertyName)
        {
            JsonElement element = JsonSerializer.SerializeToElement(value, jsonSerializerOptions);

            return new CallToolResult()
            {
                IsError = isError,

                // The text block is retained even when structured content is present; the protocol
                // asks for it so that clients that do not understand structured content still receive
                // the data.
                Content = [new TextContentBlock() { Text = element.GetRawText() }],
                StructuredContent = AsStructuredContent(element, collectionPropertyName),
            };
        }

        /// <summary>
        /// Adapts a serialized value for use as structured content, which the protocol requires to be a
        /// JSON object.
        /// </summary>
        private static JsonElement? AsStructuredContent(JsonElement element, string? collectionPropertyName)
        {
            if (element.ValueKind == JsonValueKind.Object)
            {
                return element;
            }

            if (collectionPropertyName == null)
            {
                // Anything else cannot be represented, so send only the text block rather than
                // sending structured content that violates the protocol.
                return null;
            }

            JsonObject wrapper = new JsonObject()
            {
                [collectionPropertyName] = JsonNode.Parse(element.GetRawText()),
            };

            return JsonSerializer.SerializeToElement(wrapper);
        }

        private static JsonSerializerOptions CreateDefaultJsonOptions()
        {
            return new JsonSerializerOptions()
            {
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
                Converters =
                {
                    new JsonStringEnumConverter(),
                },
            };
        }
    }
}
