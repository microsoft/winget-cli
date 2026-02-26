// -----------------------------------------------------------------------------
// <copyright file="ManifestInstaller.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.Preview
{
    using System;

    /// <summary>
    /// Class that contains the elements that represent what an installer element can contain in the manifest file.
    /// </summary>
    public class ManifestInstaller
    {
        /// <summary>
        /// Gets or sets the architecture of the installer. Values: x86, x64, arm, arm64, all.
        /// </summary>
        public string Arch { get; set; }

        /// <summary>
        /// Gets or sets the URL of the installer.
        /// </summary>
        public string Url { get; set; }

        /// <summary>
        /// Gets or sets the SHA256 hash of the installer. Required element.
        /// </summary>
        public string Sha256 { get; set; }

        /// <summary>
        /// Gets or sets the signature SHA256 for an appx/msix. Only used by appx/msix type.
        /// </summary>
        public string SignatureSha256 { get; set; }

        /// <summary>
        /// Gets or sets the localization of the installer. Empty indicates default.
        /// </summary>
        public string Language { get; set; }

        /// <summary>
        /// Gets or sets the scope. Name to be decided.
        /// </summary>
        public string Scope { get; set; }

        /// <summary>
        /// Gets or sets the Store ProductId. Only used when InstallerType is MSStore.
        /// </summary>
        public string ProductId { get; set; }

        /// <summary>
        /// Gets or sets the installer type. If present, has more precedence than root.
        /// </summary>
        public string InstallerType { get; set; }

        /// <summary>
        /// Gets or sets the installer switches. If present, has more precedence than root.
        /// </summary>
        public InstallerSwitches Switches { get; set; }

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
                   (this.Language == other.Language) &&
                   (this.Scope == other.Scope) &&
                   (this.InstallerType == other.InstallerType) &&
                   (this.Switches == other.Switches);
    }

        /// <summary>
        /// Override object.GetHashCode.
        /// </summary>
        /// <returns>resulting hash.</returns>
        public override int GetHashCode()
        {
            return (this.Arch,
                    this.Url,
                    this.SignatureSha256,
                    this.Language,
                    this.Scope,
                    this.InstallerType,
                    this.Switches).GetHashCode();
        }
    }
}
