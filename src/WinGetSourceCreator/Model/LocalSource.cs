// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
    public class LocalSource
    {
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

        public List<DynamicInstaller>? DynamicInstallers { get; set; }

        public Signature? Signature { get; set; }

        public void Validate()
        {
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

            if (this.DynamicInstallers != null)
            {
                foreach (var installer in this.DynamicInstallers)
                {
                    installer.Validate();
                }
            }

            if (this.Signature != null)
            {
                this.Signature.Validate();
            }
        }

        public string GetIndexName()
        {
            return "index.db";
        }

        public string GetSourceName(int version)
        {
            switch (version)
            {
                case 1:
                    return "source.msix";
                case 2:
                    return "source2.msix";
            }

            throw new ArgumentOutOfRangeException(nameof(version), version, "Unknown source major version");
        }
    }
}
