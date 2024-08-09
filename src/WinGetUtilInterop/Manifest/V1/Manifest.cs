// -----------------------------------------------------------------------------
// <copyright file="Manifest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;
    using System.IO;
    using Microsoft.WinGetUtil.Common;
    using YamlDotNet.Serialization;

    /// <summary>
    /// Class that defines the structure of the manifest. Uses YamlDotNet
    /// to deserialize a string or a stream and produce a manifest object.
    /// </summary>
    public class Manifest
    {
        /// <summary>
        /// Gets or sets the Id of the package.
        /// </summary>
        [YamlMember(Alias ="PackageIdentifier")]
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
        /// Gets or sets the Channel.
        /// </summary>
        public string Channel { get; set; }

        // Localization fields

        /// <summary>
        /// Gets or sets the AppMoniker.
        /// </summary>
        public string Moniker { get; set; }

        /// <summary>
        /// Gets or sets the default locale.
        /// </summary>
        public string PackageLocale { get; set; }

        /// <summary>
        /// Gets or sets the publisher in default locale.
        /// </summary>
        public string Publisher { get; set; }

        /// <summary>
        /// Gets or sets the publisher url in default locale.
        /// </summary>
        public string PublisherUrl { get; set; }

        /// <summary>
        /// Gets or sets the publisher support url in default locale.
        /// </summary>
        public string PublisherSupportUrl { get; set; }

        /// <summary>
        /// Gets or sets the default privacy url.
        /// </summary>
        public string PrivacyUrl { get; set; }

        /// <summary>
        /// Gets or sets the author in default locale.
        /// </summary>
        public string Author { get; set; }

        /// <summary>
        /// Gets or sets the package name in default locale.
        /// </summary>
        public string PackageName { get; set; }

        /// <summary>
        /// Gets or sets the package homepage url in default locale.
        /// </summary>
        public string PackageUrl { get; set; }

        /// <summary>
        /// Gets or sets the license in default locale.
        /// </summary>
        public string License { get; set; }

        /// <summary>
        /// Gets or sets the license url in default locale.
        /// </summary>
        public string LicenseUrl { get; set; }

        /// <summary>
        /// Gets or sets the copyright in default locale.
        /// </summary>
        public string Copyright { get; set; }

        /// <summary>
        /// Gets or sets the copyright url in default locale.
        /// </summary>
        public string CopyrightUrl { get; set; }

        /// <summary>
        /// Gets or sets the package short description in default locale.
        /// </summary>
        public string ShortDescription { get; set; }

        /// <summary>
        /// Gets or sets the package description in default locale.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets the Tags in default locale.
        /// </summary>
        public List<string> Tags { get; set; }

        /// <summary>
        /// Gets or sets the package agreements in default locale.
        /// </summary>
        public List<PackageAgreement> Agreements { get; set; }

        /// <summary>
        /// Gets or sets the release notes in default locale.
        /// </summary>
        public string ReleaseNotes { get; set; }

        /// <summary>
        /// Gets or sets the manifest documentation.
        /// </summary>
        public List<ManifestDocumentation> Documentations { get; set; }

        /// <summary>
        /// Gets or sets the manifest icons information.
        /// </summary>
        public List<ManifestIcon> Icons { get; set; }

        /// <summary>
        /// Gets or sets the release notes url in default locale.
        /// </summary>
        public string ReleaseNotesUrl { get; set; }

        /// <summary>
        /// Gets or sets the purchase url of the package.
        /// </summary>
        public string PurchaseUrl { get; set; }

        /// <summary>
        /// Gets or sets the installation notes.
        /// </summary>
        public string InstallationNotes { get; set; }

        // Installer fields

        /// <summary>
        /// Gets or sets the default installer locale.
        /// </summary>
        public string InstallerLocale { get; set; }

        /// <summary>
        /// Gets or sets the default list of supported platforms.
        /// </summary>
        public List<string> Platform { get; set; }

        /// <summary>
        /// Gets or sets the default minimum OS version.
        /// </summary>
        public string MinimumOSVersion { get; set; }

        /// <summary>
        /// Gets or sets the default installer type.
        /// </summary>
        public string InstallerType { get; set; }

        /// <summary>
        /// Gets or sets the default installer scope.
        /// </summary>
        public string Scope { get; set; }

        /// <summary>
        /// Gets or sets the default list of install modes.
        /// </summary>
        public List<string> InstallModes { get; set; }

        /// <summary>
        /// Gets or sets the default list of additional installer success codes.
        /// </summary>
        public List<long> InstallerSuccessCodes { get; set; }

        /// <summary>
        /// Gets or sets the default upgrade behavior.
        /// </summary>
        public string UpgradeBehavior { get; set; }

        /// <summary>
        /// Gets or sets the default list of commands.
        /// </summary>
        public List<string> Commands { get; set; }

        /// <summary>
        /// Gets or sets the default list of supported protocols.
        /// </summary>
        public List<string> Protocols { get; set; }

        /// <summary>
        /// Gets or sets the default list of supported file extensions.
        /// </summary>
        public List<string> FileExtensions { get; set; }

        /// <summary>
        /// Gets or sets the default package family name for msix packaged installers.
        /// </summary>
        public string PackageFamilyName { get; set; }

        /// <summary>
        /// Gets or sets default product code for ARP (Add/Remove Programs) installers.
        /// </summary>
        public string ProductCode { get; set; }

        /// <summary>
        /// <summary>
        /// Gets or sets the default list of capabilities. For msix only.
        /// </summary>
        public List<string> Capabilities { get; set; }

        /// <summary>
        /// Gets or sets the default list of restricted capabilities. For msix only.
        /// </summary>
        public List<string> RestrictedCapabilities { get; set; }

        /// <summary>
        /// Gets or sets the default installer dependency.
        /// </summary>
        public InstallerDependency Dependencies { get; set; }

        /// <summary>
        /// Gets or sets the default installer switches.
        /// </summary>
        [YamlMember(Alias = "InstallerSwitches")]
        public InstallerSwitches Switches { get; set; }

        /// <summary>
        /// Gets or sets the default installer markets info.
        /// </summary>
        public InstallerMarkets Markets { get; set; }

        /// <summary>
        /// Gets or sets the installation metadata.
        /// </summary>
        public InstallerInstallationMetadata InstallationMetadata { get; set; }

        /// <summary>
        /// Gets or sets the nested installer type.
        /// </summary>
        public string NestedInstallerType { get; set; }

        /// <summary>
        /// Gets or sets the nested installer files.
        /// </summary>
        public List<InstallerNestedInstallerFile> NestedInstallerFiles { get; set; }

        /// <summary>
        /// Gets or sets the unsupported arguments.
        /// </summary>
        public List<string> UnsupportedArguments { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the default installer behavior aborts terminal.
        /// </summary>
        public bool InstallerAbortsTerminal { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the default installer behavior requires explicit install location.
        /// </summary>
        public bool InstallLocationRequired { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the default installer behavior requires explicit upgrade.
        /// </summary>
        public bool RequireExplicitUpgrade { get; set; }

        /// <summary>
        /// Gets or sets the default installer release date.
        /// </summary>
        public string ReleaseDate { get; set; }

        /// <summary>
        /// Gets or sets the default list of unsupported OS architectures.
        /// </summary>
        public List<string> UnsupportedOSArchitectures { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to display install warnings.
        /// </summary>
        public bool DisplayInstallWarnings { get; set; }

        /// <summary>
        /// Gets or sets the default list of apps and features entries.
        /// </summary>
        public List<InstallerArpEntry> AppsAndFeaturesEntries { get; set; }

        /// <summary>
        /// Gets or sets the default installer elevation requirement.
        /// </summary>
        public string ElevationRequirement { get; set; }

        /// <summary>
        /// Gets or sets the default list of installer expected return codes.
        /// </summary>
        public List<InstallerExpectedReturnCode> ExpectedReturnCodes { get; set; }

        /// <summary>
        /// Gets or sets collection of ManifestInstaller. At least one is required.
        /// </summary>
        public List<ManifestInstaller> Installers { get; set; }

        /// <summary>
        /// Gets or sets collection of additional ManifestLocalization.
        /// </summary>
        public List<ManifestLocalization> Localization { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the installer is prohibited from being downloaded for offline installation.
        /// </summary>
        public bool DownloadCommandProhibited { get; set; }

        /// <summary>
        /// Gets or sets the default repair behavior.
        /// </summary>
        public string RepairBehavior { get; set; }

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
            var deserializer = Helpers.CreateDeserializer();
            return deserializer.Deserialize<Manifest>(streamReader);
        }

        /// <summary>
        /// Deserialize a string into a Manifest object.
        /// </summary>
        /// <param name="value">Manifest in string value.</param>
        /// <returns>Manifest object populated and validated.</returns>
        public static Manifest CreateManifestFromString(string value)
        {
            var deserializer = Helpers.CreateDeserializer();
            return deserializer.Deserialize<Manifest>(value);
        }

        /// <summary>
        /// Returns a List of strings containing the URIs contained within this manifest.
        /// </summary>
        /// <returns>List of strings.</returns>
        public List<string> GetURIs()
        {
            List<string> uris = new List<string>();
            if (!string.IsNullOrEmpty(this.PublisherUrl))
            {
                uris.Add(this.PublisherUrl);
            }

            if (!string.IsNullOrEmpty(this.PublisherSupportUrl))
            {
                uris.Add(this.PublisherSupportUrl);
            }

            if (!string.IsNullOrEmpty(this.PrivacyUrl))
            {
                uris.Add(this.PrivacyUrl);
            }

            if (!string.IsNullOrEmpty(this.PackageUrl))
            {
                uris.Add(this.PackageUrl);
            }

            if (!string.IsNullOrEmpty(this.LicenseUrl))
            {
                uris.Add(this.LicenseUrl);
            }

            if (!string.IsNullOrEmpty(this.CopyrightUrl))
            {
                uris.Add(this.CopyrightUrl);
            }

            if (!string.IsNullOrEmpty(this.ReleaseNotesUrl))
            {
                uris.Add(this.ReleaseNotesUrl);
            }

            if (this.Agreements != null)
            {
                foreach (var agreement in this.Agreements)
                {
                    if (!string.IsNullOrEmpty(agreement.AgreementUrl))
                    {
                        uris.Add(agreement.AgreementUrl);
                    }
                }
            }

            foreach (ManifestInstaller installer in this.Installers)
            {
                uris.AddRange(installer.GetURIs());
            }

            if (this.Localization != null)
            {
                foreach (ManifestLocalization localization in this.Localization)
                {
                    uris.AddRange(localization.GetURIs());
                }
            }

            if (this.Documentations != null)
            {
                foreach (var docs in this.Documentations)
                {
                    if (!string.IsNullOrEmpty(docs.DocumentUrl))
                    {
                        uris.Add(docs.DocumentUrl);
                    }
                }
            }

            if (this.Icons != null)
            {
                foreach (var icon in this.Icons)
                {
                    if (!string.IsNullOrEmpty(icon.IconUrl))
                    {
                        uris.Add(icon.IconUrl);
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
                   this.CompareInstallers(other.Installers);
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
