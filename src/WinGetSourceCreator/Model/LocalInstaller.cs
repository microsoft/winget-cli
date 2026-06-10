// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
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
}
