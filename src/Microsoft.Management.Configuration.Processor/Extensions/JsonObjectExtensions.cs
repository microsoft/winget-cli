// -----------------------------------------------------------------------------
// <copyright file="JsonObjectExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System.Text.Json;
    using System.Text.Json.Nodes;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Extensions for JsonObject.
    /// </summary>
    internal static class JsonObjectExtensions
    {
        /// <summary>
        /// Converts the JSON object to a ValueSet.
        /// </summary>
        /// <param name="jsonObject">The object to convert.</param>
        /// <returns>The ValueSet.</returns>
        public static ValueSet ToValueSet(this JsonObject? jsonObject)
        {
            ValueSet result = new ValueSet();

            if (jsonObject != null)
            {
                foreach (var item in jsonObject)
                {
                    result.Add(item.Key, ToValue(item.Value));
                }
            }

            return result;
        }

        private static object? ToValue(JsonNode? node) => node switch
        {
            JsonObject obj => obj.ToValueSet(),
            JsonArray array => ToValueSet(array),
            JsonValue value => ToValue(value),
            _ => null,
        };

        private static ValueSet ToValueSet(JsonArray array)
        {
            ValueSet result = new ValueSet();
            result.Add(ValueSetExtensions.TreatAsArray, true);

            int index = 0;
            foreach (var item in array)
            {
                result.Add(index.ToString(), ToValue(item));
                ++index;
            }

            return result;
        }

        private static object? ToValue(JsonValue value) => value.GetValueKind() switch
        {
            JsonValueKind.Null => null,
            JsonValueKind.Undefined => null,
            JsonValueKind.String => value.GetValue<string>(),
            JsonValueKind.Number => value.GetValue<long>(),
            JsonValueKind.True => true,
            JsonValueKind.False => false,
            _ => throw new System.NotImplementedException("Unexpected default case")
        };
    }
}
