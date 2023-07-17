// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
    public class SourceInstaller : Installer
    {
        public SourceInstaller(string installerFile, Installer installer)
        {
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

        public string InstallerFile { get; private set; }
    }
}
