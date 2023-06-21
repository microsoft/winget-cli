// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using System.IO.Compression;

namespace Microsoft.WinGetSourceCreator
{
    public enum InstallerType
    {
        Msix,
        Exe,
        Msi,
        Zip,
    }

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

        public List<DynamicInstaller>? DynamicInstallers { get; set; }

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

    public abstract class Installer
    {
        public InstallerType Type { get; set; }

        // The name of the installer when it copied or created.
        public string Name { get; set; } = string.Empty;

        // The identifying token of the installer in the manifests.
        public string? HashToken { get; set; }

        // An optional token relevant to the installer.
        // If the installer is an msix, this is the token used for the appx signature hash.
        public string? SignatureToken { get; set; }

        public Signature? Signature { get; set; }

        public bool SkipSignature { get; set; }

        protected void Validate()
        {
            if (string.IsNullOrEmpty(this.Name))
            {
                throw new ArgumentNullException(nameof(this.Name));
            }

            if (this.Signature != null)
            {
                this.Signature.Validate();
            }

            if (this.Type != InstallerType.Msix && !string.IsNullOrEmpty(this.SignatureToken))
            {
                throw new Exception($"{nameof(this.SignatureToken)} can only be used for MSIX");
            }
        }
    }

    public class SourceInstaller : Installer
    {
        public SourceInstaller(string installerFile, Installer installer)
        {
            var properties = this.GetType().GetProperties();

            foreach (var installerProperty in installer.GetType().GetProperties())
            {
                var toProperty = this.GetType().GetProperty(installerProperty.Name);
                if (toProperty != null)
                {
                    toProperty.SetValue(this, installerProperty.GetValue(installer));
                }
            }

            this.InstallerFile = Path.Combine(installerFile, this.Name);
        }

        public string InstallerFile { get;  private set; }
    }

    public class LocalInstaller : Installer
    {
        // The full path of the installer.
        // Gets copied to the output directory and optionally signed.
        public string Input { get; set; } = string.Empty;

        internal new void Validate()
        {
            base.Validate();
            if (string.IsNullOrEmpty(this.Input))
            {
                throw new ArgumentNullException(nameof(this.Input));
            }

            if (!File.Exists(this.Input))
            {
                throw new FileNotFoundException(this.Input);
            }
        }
    }

    public class DynamicInstaller : Installer
    {
        // Input depends on the Type.
        // For zip it is the directories or files that need to included in the zip
        public List<string> Input { get; set; } = new List<string>();

        internal new void Validate()
        {
            base.Validate();
        }

        public void Create(string outputFile)
        {
            if (this.Type == InstallerType.Zip)
            {
                CreateZipInstaller(outputFile);
            }
            else
            {
                throw new NotImplementedException();
            }
        }

        private void CreateZipInstaller(string outputFile)
        {
            var tmpPath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
            if (Directory.Exists(tmpPath))
            {
                Directory.Delete(tmpPath, true);
            }
            Directory.CreateDirectory(tmpPath);

            foreach (var i in this.Input)
            {
                if (File.Exists(i))
                {
                    File.Copy(i, Path.Combine(tmpPath, Path.GetFileName(i)), true);
                }
                else if (Directory.Exists(i))
                {
                    Helpers.CopyDirectory(i, tmpPath);
                }
                else
                {
                    throw new InvalidOperationException(i);
                }
            }

            ZipFile.CreateFromDirectory(tmpPath, outputFile);

            try
            {
                Directory.Delete(tmpPath, true);
            }
            catch (Exception)
            {
            }
        }
    }

    public class Signature
    {
        // Full path of the certificate used to sign the package and installers
        public string CertFile { get; set; } = string.Empty;

        public string? Password { get; set; }

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
