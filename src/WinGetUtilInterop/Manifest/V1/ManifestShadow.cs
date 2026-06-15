// -----------------------------------------------------------------------------
// <copyright file="ManifestShadow.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Manifest.V1
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using Microsoft.WinGetUtil.Common;
    using Microsoft.WinGetUtil.Exceptions;
    using Microsoft.WinGetUtil.Models.V1;
    using YamlDotNet.Serialization;

    /// <summary>
    /// Model for manifest type shadow.
    /// </summary>
    public sealed class ManifestShadow
    {
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
        /// Gets or sets the default locale.
        /// </summary>
        public string PackageLocale { get; set; }

        /// <summary>
        /// Gets or sets the manifest icons information.
        /// </summary>
        public List<ManifestIcon> Icons { get; set; }

        /// <summary>
        /// Gets or sets collection of additional ManifestShadowLocalization.
        /// </summary>
        public List<ManifestShadowLocalization> Localization { get; set; }

        /// <summary>
        /// Creates a shadow manifest.
        /// Assigns the correct ManifestType.
        /// </summary>
        /// <returns>A shadow manifest.</returns>
        public static ManifestShadow CreateManifest()
        {
            return new ManifestShadow()
            {
                ManifestType = "shadow",
            };
        }

        /// <summary>
        /// Deserialize a stream reader into a ManifestShadow object.
        /// </summary>
        /// <param name="filePath">file path.</param>
        /// <returns>ManifestShadow object populated and validated.</returns>
        public static ManifestShadow CreateManifestFromPath(string filePath)
        {
            using (StreamReader streamReader = new StreamReader(filePath))
            {
                return CreateManifestFromStreamReader(streamReader);
            }
        }

        /// <summary>
        /// Deserialize a stream into a ManifestShadow object.
        /// </summary>
        /// <param name="stream">Manifest stream.</param>
        /// <returns>ManifestShadow object populated and validated.</returns>
        public static ManifestShadow CreateManifestFromStream(Stream stream)
        {
            using (StreamReader streamReader = new StreamReader(stream))
            {
                return CreateManifestFromStreamReader(streamReader);
            }
        }

        /// <summary>
        /// Deserialize a stream reader into a ManifestShadow object.
        /// </summary>
        /// <param name="streamReader">stream reader.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static ManifestShadow CreateManifestFromStreamReader(StreamReader streamReader)
        {
            streamReader.BaseStream.Seek(0, SeekOrigin.Begin);
            var deserializer = Helpers.CreateDeserializer();
            var shadow = deserializer.Deserialize<ManifestShadow>(streamReader);
            shadow.Validate();
            return shadow;
        }

        /// <summary>
        /// Deserialize a string into a ManifestShadow object.
        /// </summary>
        /// <param name="value">Manifest in string value.</param>
        /// <returns>ManifestShadow object populated and validated.</returns>
        public static ManifestShadow CreateManifestFromString(string value)
        {
            var deserializer = Helpers.CreateDeserializer();
            var shadow = deserializer.Deserialize<ManifestShadow>(value);
            shadow.Validate();
            return shadow;
        }

        /// <summary>
        /// Serializes the shadow manifest.
        /// </summary>
        /// <returns>Serialized shadow manifest as string.</returns>
        public string Serialize()
        {
            this.Validate();
            var serializer = Helpers.CreateSerializer();
            return serializer.Serialize(this);
        }

        private void Validate()
        {
            if (this.ManifestType != "shadow")
            {
                throw new WinGetManifestException("Invalid shadow manifest");
            }

            if (string.IsNullOrEmpty(this.Id))
            {
                throw new ArgumentNullException(nameof(this.Id));
            }

            if (string.IsNullOrEmpty(this.Version))
            {
                throw new ArgumentNullException(nameof(this.Version));
            }

            if (string.IsNullOrEmpty(this.ManifestVersion))
            {
                throw new ArgumentNullException(nameof(this.ManifestVersion));
            }
        }
    }
}
