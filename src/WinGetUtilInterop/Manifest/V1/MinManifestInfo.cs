// -----------------------------------------------------------------------------
// <copyright file="MinManifestInfo.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.IO;
    using YamlDotNet.Serialization;
    using YamlDotNet.Serialization.NamingConventions;

    /// <summary>
    /// Class that contains minimum manifest info for each single manifest in a multi file manifest.
    /// Used in validation pipeline to check multi file manifest consistency.
    /// </summary>
    public class MinManifestInfo
    {
        private const string ManifestTypeSingleton = "singleton";
        private const string ManifestTypeLocale = "locale";
        private const string ManifestTypeDefaultLocale = "defaultLocale";

        private const string ValidationErrorMissingField = "Manifest file missing required field: ";
        private const string ValidationErrorFailedManifestCreation = "Failed to create Manifest object from manifest file.";

        /// <summary>
        /// Gets or sets the Id of the package.
        /// </summary>
        [YamlMember(Alias = "PackageIdentifier")]
        public string Id { get; set; }

        /// <summary>
        /// Gets or sets the version of the package.
        /// </summary>
        [YamlMember(Alias = "PackageVersion")]
        public string Version { get; set; }

        /// <summary>
        /// Gets or sets the manifest type.
        /// </summary>
        public string ManifestType { get; set; }

        /// <summary>
        /// Gets or sets the manifest version.
        /// </summary>
        public string ManifestVersion { get; set; }

        /// <summary>
        /// Gets or sets the package locale. Used in locale manifest in a multi file manifest.
        /// </summary>
        public string PackageLocale { get; set; }

        /// <summary>
        /// Deserialize a stream reader into a Manifest object.
        /// </summary>
        /// <param name="filePath">file path.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static MinManifestInfo CreateManifestInfoFromPath(string filePath)
        {
            using (StreamReader streamReader = new StreamReader(filePath))
            {
                return MinManifestInfo.CreateManifestInfoFromStreamReader(streamReader);
            }
        }

        /// <summary>
        /// Deserialize a stream into a Manifest object.
        /// </summary>
        /// <param name="stream">Manifest stream.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static MinManifestInfo CreateManifestInfoFromStream(Stream stream)
        {
            using (StreamReader streamReader = new StreamReader(stream))
            {
                return MinManifestInfo.CreateManifestInfoFromStreamReader(streamReader);
            }
        }

        /// <summary>
        /// Deserialize a stream reader into a Manifest object.
        /// </summary>
        /// <param name="streamReader">stream reader.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static MinManifestInfo CreateManifestInfoFromStreamReader(StreamReader streamReader)
        {
            streamReader.BaseStream.Seek(0, SeekOrigin.Begin);
            var deserializer = CreateDeserializer();
            var result = deserializer.Deserialize<MinManifestInfo>(streamReader);
            ValidateManifestInfo(result);
            return result;
        }

        /// <summary>
        /// Deserialize a string into a Manifest object.
        /// </summary>
        /// <param name="value">Manifest in string value.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static MinManifestInfo CreateManifestInfoFromString(string value)
        {
            var deserializer = CreateDeserializer();
            var result = deserializer.Deserialize<MinManifestInfo>(value);
            ValidateManifestInfo(result);
            return result;
        }

        /// <summary>
        /// Helper to deserialize the manifest.
        /// </summary>
        /// <returns>IDeserializer object.</returns>
        private static IDeserializer CreateDeserializer()
        {
            var deserializer = new DeserializerBuilder().
                WithNamingConvention(PascalCaseNamingConvention.Instance).
                IgnoreUnmatchedProperties();
            return deserializer.Build();
        }

        /// <summary>
        /// Validate manifest info integrity.
        /// </summary>
        private static void ValidateManifestInfo(MinManifestInfo manifestInfo)
        {
            if (manifestInfo == null)
            {
                throw new InvalidDataException(ValidationErrorFailedManifestCreation);
            }

            if (string.IsNullOrEmpty(manifestInfo.Id))
            {
                throw new InvalidDataException(ValidationErrorMissingField + "PackageIdentifier");
            }

            if (string.IsNullOrEmpty(manifestInfo.Version))
            {
                throw new InvalidDataException(ValidationErrorMissingField + "PackageVersion");
            }

            if (string.IsNullOrEmpty(manifestInfo.ManifestType))
            {
                throw new InvalidDataException(ValidationErrorMissingField + "ManifestType");
            }

            if (string.IsNullOrEmpty(manifestInfo.ManifestVersion))
            {
                throw new InvalidDataException(ValidationErrorMissingField + "ManifestVersion");
            }

            if (string.IsNullOrEmpty(manifestInfo.PackageLocale) &&
                (manifestInfo.ManifestType == ManifestTypeDefaultLocale ||
                manifestInfo.ManifestType == ManifestTypeLocale ||
                manifestInfo.ManifestType == ManifestTypeSingleton))
            {
                throw new InvalidDataException(ValidationErrorMissingField + "PackageLocale");
            }
        }
    }
}
