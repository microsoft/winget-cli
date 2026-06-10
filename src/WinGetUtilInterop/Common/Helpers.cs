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
            return new DeserializerBuilder()
                .WithNamingConvention(PascalCaseNamingConvention.Instance)
                .IgnoreUnmatchedProperties()
                .Build();
        }

        /// <summary>
        /// Helper to serialize the manifest.
        /// </summary>
        /// <returns>ISerializer object.</returns>
        public static ISerializer CreateSerializer()
        {
            return new SerializerBuilder()
                .WithNamingConvention(PascalCaseNamingConvention.Instance)
                .Build();
        }
    }
}
