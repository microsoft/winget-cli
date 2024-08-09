// -----------------------------------------------------------------------------
// <copyright file="Manifest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.Preview
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using YamlDotNet.Serialization;
    using YamlDotNet.Serialization.NamingConventions;

    /// <summary>
    /// Class that defines the structure of the manifest. Uses YamlDotNet
    /// to deserialize a string or a stream and produce a manifest object.
    /// </summary>
    public class Manifest
    {
        /// <summary>
        /// Gets or sets the Id of the manifest.
        /// </summary>
        public string Id { get; set; }

        /// <summary>
        /// Gets or sets the name of the package.
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the version of the package.
        /// </summary>
        public string Version { get; set; }

        /// <summary>
        /// Gets or sets the publisher.
        /// </summary>
        public string Publisher { get; set; }

        /// <summary>
        /// Gets or sets the AppMoniker.
        /// </summary>
        public string AppMoniker { get; set; }

        /// <summary>
        /// Gets or sets the Channel.
        /// </summary>
        public string Channel { get; set; }

        /// <summary>
        /// Gets or sets the Author of package.
        /// </summary>
        public string Author { get; set; }

        /// <summary>
        /// Gets or sets the License.
        /// </summary>
        public string License { get; set; }

        /// <summary>
        /// Gets or sets the MinOSVersion.
        /// </summary>
        public string MinOSVersion { get; set; }

        /// <summary>
        /// Gets or sets the Tags. Comma separated values.
        /// </summary>
        public string Tags { get; set; }

        /// <summary>
        /// Gets or sets the Commands. Comma separated values.
        /// </summary>
        public string Commands { get; set; }

        /// <summary>
        /// Gets or sets the Protocols. Comma separated values.
        /// </summary>
        public string Protocols { get; set; }

        /// <summary>
        /// Gets or sets the FileExtensions. Comma separated values.
        /// </summary>
        public string FileExtensions { get; set; }

        /// <summary>
        /// Gets or sets the InstallerType of the package. Must be one
        /// of the values of ValidInstallerTypes. IManifestInstallerDefaults.
        /// An InstallerType in the root of the manifest is required if
        /// InstallerType is not defined in an ManifestInstaller entry.
        /// </summary>
        public string InstallerType { get; set; }

        /// <summary>
        /// Gets or sets Description. IManifestLocalization.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets Homepage. IManifestLocalization.
        /// </summary>
        public string Homepage { get; set; }

        /// <summary>
        /// Gets or sets LicenseUrl. IManifestLocalization.
        /// </summary>
        public string LicenseUrl { get; set; }

        /// <summary>
        /// Gets or sets InstallerSwitches. IManifestInstallerDefaults.
        /// </summary>
        public InstallerSwitches Switches { get; set; }

        /// <summary>
        /// Gets or sets collection of ManifestInstaller. At least one is required.
        /// </summary>
        public List<ManifestInstaller> Installers { get; set; }

        /// <summary>
        /// Gets or sets collection of ManifestLocalization.
        /// </summary>
        public List<ManifestLocalization> Localization { get; set; }

        /// <summary>
        /// Gets or sets the manifest version.
        /// </summary>
        public string ManifestVersion { get; set; }

        /// <summary>
        /// Deserialize a stream into a Manifest object.
        /// </summary>
        /// <param name="stream">Manifest stream.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static Manifest CreateManifestFromStream(Stream stream)
        {
            using (StreamReader streamReader = new StreamReader(stream))
            {
                return Manifest.CreateManifestFromStreamReader(streamReader);
            }
        }

        /// <summary>
        /// Deserialize a stream reader into a Manifest object.
        /// </summary>
        /// <param name="streamReader">stream reader.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static Manifest CreateManifestFromStreamReader(StreamReader streamReader)
        {
            streamReader.BaseStream.Seek(0, SeekOrigin.Begin);
            var deserializer = CreateDeserializer();
            return deserializer.Deserialize<Manifest>(streamReader);
        }

        /// <summary>
        /// Deserialize a stream reader into a Manifest object.
        /// </summary>
        /// <param name="filePath">file path.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static Manifest CreateManifestFromPath(string filePath)
        {
            using (StreamReader streamReader = new StreamReader(filePath))
            {
                return Manifest.CreateManifestFromStreamReader(streamReader);
            }
        }

        /// <summary>
        /// Deserialize a string into a Manifest object.
        /// </summary>
        /// <param name="value">Manifest in string value.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static Manifest CreateManifestFromString(string value)
        {
            var deserializer = CreateDeserializer();
            return deserializer.Deserialize<Manifest>(value);
        }

        /// <summary>
        /// Returns a List of strings containing the URIs contained within this manifest.
        /// </summary>
        /// <returns>List of strings.</returns>
        public List<string> GetURIs()
        {
            List<string> uris = new List<string>();
            if (!string.IsNullOrEmpty(this.Homepage))
            {
                uris.Add(this.Homepage);
            }

            if (!string.IsNullOrEmpty(this.LicenseUrl))
            {
                uris.Add(this.LicenseUrl);
            }

            foreach (ManifestInstaller installer in this.Installers)
            {
                if (!string.IsNullOrEmpty(installer.Url))
                {
                    uris.Add(installer.Url);
                }
            }

            if (this.Localization != null)
            {
                foreach (ManifestLocalization localization in this.Localization)
                {
                    if (!string.IsNullOrEmpty(localization.LicenseUrl))
                    {
                        uris.Add(localization.LicenseUrl);
                    }

                    if (!string.IsNullOrEmpty(localization.Homepage))
                    {
                        uris.Add(localization.Homepage);
                    }
                }
            }

            return uris;
        }

        /// <summary>
        /// Checks if two manifests are equivalent.
        /// </summary>
        /// <param name="other">other object.</param>
        /// <returns>boolean indicating if the objects are equals.</returns>
        public bool IsManifestEquivalent(Manifest other)
        {
            // If parameter is null, return false.
            if (ReferenceEquals(other, null))
            {
                return false;
            }

            // Optimization for a common success case.
            if (ReferenceEquals(this, other))
            {
                return true;
            }

            // If run-time types are not exactly the same, return false.
            if (this.GetType() != other.GetType())
            {
                return false;
            }

            // Equality of Manifest consist on only these properties.
            return (this.Id == other.Id) &&
                   (this.Version == other.Version) &&
                   (this.Publisher == other.Publisher) &&
                   (this.InstallerType == other.InstallerType) &&
                   (this.Switches == other.Switches) &&
                   this.CompareInstallers(other.Installers);
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

        private bool CompareInstallers(List<ManifestInstaller> installers)
        {
            ISet<ManifestInstaller> first =
                new HashSet<ManifestInstaller>(
                    this.Installers != null ?
                    this.Installers :
                    new List<ManifestInstaller>());

            return first.SetEquals(installers != null
                ? installers : new List<ManifestInstaller>());
        }
    }
}
