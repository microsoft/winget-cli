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

        // RFC 3161 timestamp server URL (e.g. http://timestamp.digicert.com).
        // When set, a countersignature timestamp is added so the signature remains
        // valid after the signing certificate expires.
        public string? TimestampServer { get; set; }

        internal void Validate()
        {
            if (string.IsNullOrEmpty(this.CertFile))
            {
                throw new ArgumentNullException(nameof(this.CertFile));
            }
        }
    }
}
