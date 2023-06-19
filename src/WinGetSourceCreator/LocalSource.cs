﻿// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.WinGetSourceCreator
{
    public class LocalSource
    {
        // The name of the local source.
        // Output msix package and index are are named by this.
        public string Name { get; set; } = string.Empty;

        // Full path of the input appx manifest.
        // Will be used to generate the package.
        public string AppxManifest { get; set; } = string.Empty;

        // The working directory where manifest will copied and referenced by the index.
        public string WorkingDirectory { get; set; } = string.Empty;

        // Input manifests.
        // If it is a file the manifest gets copied to the input directory.
        // If it is a directory the manifests will be copied preserving the sub dirs structure.
        public List<string> LocalManifests { get; set; } = new();

        public List<LocalInstaller>? LocalInstallers { get; set; }

        public Signature? Signature { get; set; }

        public void Validate()
        {
            if (string.IsNullOrEmpty(this.Name))
            {
                throw new ArgumentNullException(nameof(this.Name));
            }

            if (string.IsNullOrEmpty(this.AppxManifest))
            {
                throw new ArgumentNullException(nameof(this.AppxManifest));
            }

            if (string.IsNullOrEmpty(this.WorkingDirectory))
            {
                throw new ArgumentNullException(nameof(this.WorkingDirectory));
            }

            if (this.LocalManifests.Count == 0)
            {
                throw new ArgumentException(nameof(this.LocalManifests));
            }

            if (this.LocalInstallers != null)
            {
                foreach (var installer in this.LocalInstallers)
                {
                    installer.Validate();
                }
            }

            if (this.Signature != null)
            {
                this.Signature.Validate();

                // Top level requires appx info
                if (this.Signature.Name == null)
                {
                    throw new ArgumentNullException(nameof(this.Signature.Name));
                }

                if (this.Signature.Publisher == null)
                {
                    throw new ArgumentNullException(nameof(this.Signature.Publisher));
                }
            }
        }

        public string GetIndexName()
        {
            return $"{this.Name}.db";
        }

        public string GetSourceName()
        {
            return $"{this.Name}.msix";
        }
    }

    public class LocalInstaller
    {
        // The full path of the installer.
        // Gets copied to the output directory and optionally signed.
        public string InstallerFile { get; set; } = string.Empty;

        // The identifying token of the installer in the manifests.
        public string Token { get; set; } = string.Empty;

        // An optional token relevant to the installer.
        // If the installer is an msix, this is the token used for the appx signature hash.
        public string? MiscToken { get; set; }

        public Signature? Signature { get; set; }

        internal void Validate()
        {
            if (string.IsNullOrEmpty(this.InstallerFile))
            {
                throw new ArgumentNullException(nameof(this.InstallerFile));
            }

            if (string.IsNullOrEmpty(this.Token))
            {
                throw new ArgumentNullException(nameof(this.Token));
            }

            if (!File.Exists(InstallerFile))
            {
                throw new FileNotFoundException(InstallerFile);
            }

            if (this.Signature != null)
            {
                this.Signature.Validate();
            }
        }
        internal bool IsMsix()
        {
            string ext = Path.GetExtension(InstallerFile);
            return string.Compare(ext, ".msix", true) == 0 || string.Compare(ext, ".appx", true) == 0;
        }
    }

    public class Signature
    {
        // Full path of the certificate used to sign the package and installers
        public string CertFile { get; set; } = string.Empty;

        // The name for the AppxPackage Identity Name property.
        public string? Name { get; set; }

        // The publisher for the AppxPackage Identity Name property.
        public string? Publisher { get; set; }

        internal void Validate()
        {
            if (string.IsNullOrEmpty(this.CertFile))
            {
                throw new ArgumentNullException(nameof(this.CertFile));
            }
        }
    }
}
