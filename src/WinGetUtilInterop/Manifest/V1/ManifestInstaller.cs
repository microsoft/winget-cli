// -----------------------------------------------------------------------------
// <copyright file="ManifestInstaller.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;
    using YamlDotNet.Serialization;

    /// <summary>
    /// Class that contains the elements that represent what an installer element can contain in the manifest file.
    /// </summary>
    public class ManifestInstaller
    {
        /// <summary>
        /// Gets or sets the architecture of the installer. Values: x86, x64, arm, arm64, neutral.
        /// </summary>
        [YamlMember(Alias = "Architecture")]
        public string Arch { get; set; }

        /// <summary>
        /// Gets or sets the URL of the installer.
        /// </summary>
        [YamlMember(Alias = "InstallerUrl")]
        public string Url { get; set; }

        /// <summary>
        /// Gets or sets the SHA256 hash of the installer. Required element.
        /// </summary>
        [YamlMember(Alias = "InstallerSha256")]
        public string Sha256 { get; set; }

        /// <summary>
        /// Gets or sets the signature SHA256 for an appx/msix. Only used by appx/msix type.
        /// </summary>
        public string SignatureSha256 { get; set; }

        /// <summary>
        /// Gets or sets the installer locale.
        /// </summary>
        public string InstallerLocale { get; set; }

        /// <summary>
        /// Gets or sets the list of supported platforms.
        /// </summary>
        public List<string> Platform { get; set; }

        /// <summary>
        /// Gets or sets the minimum OS version.
        /// </summary>
        public string MinimumOSVersion { get; set; }

        /// <summary>
        /// Gets or sets the installer type. If present, has more precedence than root.
        /// </summary>
        public string InstallerType { get; set; }

        /// <summary>
        /// Gets or sets the installer scope.
        /// </summary>
        public string Scope { get; set; }

        /// <summary>
        /// Gets or sets the list of install modes.
        /// </summary>
        public List<string> InstallModes { get; set; }

        /// <summary>
        /// Gets or sets the list of additional installer success codes.
        /// </summary>
        public List<long> InstallerSuccessCodes { get; set; }

        /// <summary>
        /// Gets or sets the upgrade behavior.
        /// </summary>
        public string UpgradeBehavior { get; set; }

        /// <summary>
        /// Gets or sets the list of commands.
        /// </summary>
        public List<string> Commands { get; set; }

        /// <summary>
        /// Gets or sets the list of supported protocols.
        /// </summary>
        public List<string> Protocols { get; set; }

        /// <summary>
        /// Gets or sets the list of supported file extensions.
        /// </summary>
        public List<string> FileExtensions { get; set; }

        /// <summary>
        /// Gets or sets the package family name for msix packaged installers.
        /// </summary>
        public string PackageFamilyName { get; set; }

        /// <summary>
        /// Gets or sets product code for ARP (Add/Remove Programs) installers.
        /// </summary>
        public string ProductCode { get; set; }

        /// <summary>
        /// Gets or sets the list of capabilities. For msix only.
        /// </summary>
        public List<string> Capabilities { get; set; }

        /// <summary>
        /// Gets or sets the list of restricted capabilities. For msix only.
        /// </summary>
        public List<string> RestrictedCapabilities { get; set; }

        /// <summary>
        /// Gets or sets the installer dependency.
        /// </summary>
        public InstallerDependency Dependencies { get; set; }

        /// <summary>
        /// Gets or sets the installer switches. If present, has more precedence than root.
        /// </summary>
        [YamlMember(Alias = "InstallerSwitches")]
        public InstallerSwitches Switches { get; set; }

        /// <summary>
        /// Gets or sets the installer markets info.
        /// </summary>
        public InstallerMarkets Markets { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the installer behavior aborts terminal.
        /// </summary>
        public bool InstallerAbortsTerminal { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the installer behavior requires explicit install location.
        /// </summary>
        public bool InstallLocationRequired { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the installer behavior requires explicit upgrade.
        /// </summary>
        public bool RequireExplicitUpgrade { get; set; }

        /// <summary>
        /// Gets or sets the installer release date.
        /// </summary>
        public string ReleaseDate { get; set; }

        /// <summary>
        /// Gets or sets the list of unsupported OS architectures.
        /// </summary>
        public List<string> UnsupportedOSArchitectures { get; set; }

        /// <summary>
        /// Gets or sets the list of apps and features entries.
        /// </summary>
        public List<InstallerArpEntry> AppsAndFeaturesEntries { get; set; }

        /// <summary>
        /// Gets or sets the installer elevation requirement.
        /// </summary>
        public string ElevationRequirement { get; set; }

        /// <summary>
        /// Gets or sets the list of installer expected return codes.
        /// </summary>
        public List<InstallerExpectedReturnCode> ExpectedReturnCodes { get; set; }

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
        /// Gets or sets a value indicating whether to display install warnings.
        /// </summary>
        public bool DisplayInstallWarnings { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether the installer is prohibited from being downloaded for offline installation.
        /// </summary>
        public bool DownloadCommandProhibited { get; set; }

        /// <summary>
        /// Gets or sets the repair behavior.
        /// </summary>
        public string RepairBehavior { get; set; }

        /// <summary>
        /// Returns a List of strings containing the URIs contained within this installer.
        /// </summary>
        /// <returns>List of strings.</returns>
        public List<string> GetURIs()
        {
            List<string> uris = new List<string>();
            if (!string.IsNullOrEmpty(this.Url))
            {
                uris.Add(this.Url);
            }

            return uris;
        }

        /// <summary>
        /// Override object.Equals.
        /// </summary>
        /// <param name="other">other object.</param>
        /// <returns>boolean indicating if the objects are equals.</returns>
        public override bool Equals(object other)
        {
            return (other is ManifestInstaller) && this.Equals(other as ManifestInstaller);
        }

        /// <summary>
        /// Implemented IEquitable.
        /// </summary>
        /// <param name="other">other object.</param>
        /// <returns>boolean indicating if the objects are equals.</returns>
        public bool Equals(ManifestInstaller other)
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

            return (this.Arch == other.Arch) &&
                   (this.Url == other.Url) &&
                   (this.Sha256 == other.Sha256) &&
                   (this.SignatureSha256 == other.SignatureSha256) &&
                   (this.InstallerLocale == other.InstallerLocale) &&
                   (this.Scope == other.Scope) &&
                   (this.InstallerType == other.InstallerType) &&
                   (this.Switches == other.Switches) &&
                   (this.NestedInstallerType == other.NestedInstallerType) &&
                   (this.NestedInstallerFiles == other.NestedInstallerFiles);
    }

        /// <summary>
        /// Override object.GetHashCode.
        /// </summary>
        /// <returns>resulting hash.</returns>
        public override int GetHashCode()
        {
            return (this.Arch,
                    this.Url,
                    this.Sha256,
                    this.SignatureSha256,
                    this.InstallerLocale,
                    this.Scope,
                    this.InstallerType,
                    this.Switches,
                    this.NestedInstallerType,
                    this.NestedInstallerFiles).GetHashCode();
        }
    }
}
