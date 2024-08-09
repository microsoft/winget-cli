// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
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
