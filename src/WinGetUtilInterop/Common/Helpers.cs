// -----------------------------------------------------------------------------
// <copyright file="Helpers.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Common
{
    using YamlDotNet.Serialization;
    using YamlDotNet.Serialization.NamingConventions;

    /// <summary>
    /// Helpers.
    /// </summary>
    internal static class Helpers
    {
        /// <summary>
        /// Helper to deserialize the manifest.
        /// </summary>
        /// <returns>IDeserializer object.</returns>
        public static IDeserializer CreateDeserializer()
        {
            var deserializer = new DeserializerBuilder().
                WithNamingConvention(PascalCaseNamingConvention.Instance).
                IgnoreUnmatchedProperties();
            return deserializer.Build();
        }
    }
}
