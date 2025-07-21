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
    using System.Text.Json.Serialization;
    using WinGetMCPServer.Exceptions;
    using static System.Runtime.InteropServices.JavaScript.JSType;

    /// <summary>
    /// Contains reusable responses for tools.
    /// </summary>
    internal static class ToolResponse
    {
        /// <summary>
        /// Checks whether the server is disabled by group policy.
        /// </summary>
        public static void CheckGroupPolicy()
        {
            if (!GroupPolicy.GetInstance().IsEnabled(Policy.McpServer))
            {
                throw new ToolResponseException(new CallToolResponse()
                {
                    IsError = true,
                    Content = [new Content() { Text = "The Windows Package Manager MCP server is disabled by group policy." }]
                });
            }
        }

        /// <summary>
        /// Constructs a response from an object.
        /// </summary>
        /// <param name="value">The object to return in the response.</param>
        /// <param name="isError">Whether or not the response is an error.</param>
        /// <returns>The response.</returns>
        public static CallToolResponse FromObject(object value, bool isError = false)
        {
            return FromObject(value, isError, GetDefaultJsonOptions());
        }

        /// <summary>
        /// Constructs a response from an object.
        /// </summary>
        /// <param name="value">The object to return in the response.</param>
        /// <param name="isError">Whether or not the response is an error.</param>
        /// <param name="jsonSerializerOptions">The JSON serializer options for serializing the object.</param>
        /// <returns>The response.</returns>
        public static CallToolResponse FromObject(object value, bool isError, JsonSerializerOptions jsonSerializerOptions)
        {
            return new CallToolResponse()
            {
                IsError = isError,
                Content = [new Content() { Text = JsonSerializer.Serialize(value, GetDefaultJsonOptions()) }]
            };
        }

        /// <summary>
        /// Gets the default serialization options.
        /// </summary>
        /// <returns>The default serialization options.</returns>
        public static JsonSerializerOptions GetDefaultJsonOptions()
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
