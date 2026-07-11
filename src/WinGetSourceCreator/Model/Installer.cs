// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
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
}
