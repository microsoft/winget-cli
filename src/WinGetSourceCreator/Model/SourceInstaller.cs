// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
    public class SourceInstaller : Installer
    {
        public SourceInstaller(string workingDirectory, DynamicInstaller installer)
        {
            this.Initialize(installer);
            this.InstallerFile = installer.Create(workingDirectory);
        }

        public SourceInstaller(string workingDirectory, LocalInstaller installer)
        {
            this.Initialize(installer);
            this.InstallerFile = Path.Combine(workingDirectory, this.Name);
            var parent = Path.GetDirectoryName(this.InstallerFile);
            if (!string.IsNullOrEmpty(parent))
            {
                Directory.CreateDirectory(parent);
            }
            File.Copy(installer.Input, this.InstallerFile, true);
        }

        public string InstallerFile { get; private set; }

        private void Initialize(Installer installer)
        {
            foreach (var installerProperty in installer.GetType().GetProperties())
            {
                var toProperty = this.GetType().GetProperty(installerProperty.Name);
                if (toProperty != null)
                {
                    toProperty.SetValue(this, installerProperty.GetValue(installer));
                }
            }
        }
    }
}
