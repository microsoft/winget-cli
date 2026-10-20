// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace WinGetSourceCreator.Model
{
    // Describes a delta capable source to publish alongside the ordinary one.
    //
    // It is published as its own source rather than layered onto the existing packages so that a
    // client that knows nothing about deltas keeps seeing exactly what it saw before.
    public class DeltaSettings
    {
        // Directory, relative to the working directory, that the delta source is published under.
        // This is also what a client is pointed at, so everything the source offers lives below it.
        public string RelativeDirectory { get; set; } = "deltaSource";

        // Identity name carried by the full index.
        public string IdentityName { get; set; } = string.Empty;

        // Identity name carried by the delta, which has to differ from the above so that a client
        // can acquire one without the other.
        public string DeltaIdentityName { get; set; } = string.Empty;

        // Identity name carried by the baseline. A baseline is a full index in content, but it is
        // published as a package in its own right: a client on the delta holds a baseline and no
        // full index, while one that is not holds the opposite, and a store that addresses packages
        // by identity could not tell the two apart if they shared one.
        public string BaselineIdentityName { get; set; } = string.Empty;

        // The version of the full index and of the delta that describes it.
        public string Version { get; set; } = string.Empty;

        // The baseline is published at an older version than the full index, because a baseline is
        // captured once and then described by a succession of deltas. The two no longer share an
        // identity, so this is realism rather than a constraint the client depends on.
        public string BaselineVersion { get; set; } = string.Empty;

        // Where the baseline is published, relative to the source's own location. The delta records
        // this, so it is what a client will ask for.
        public string BaselineRelativePath { get; set; } = "baseline/source2.msix";

        // Manifest file names left out of the baseline. Without these the delta would be empty and
        // a test could not tell a merged result apart from either half read on its own.
        public List<string> ManifestsNotInBaseline { get; set; } = new();

        public void Validate()
        {
            if (string.IsNullOrEmpty(this.RelativeDirectory))
            {
                throw new ArgumentNullException(nameof(this.RelativeDirectory));
            }

            if (string.IsNullOrEmpty(this.IdentityName))
            {
                throw new ArgumentNullException(nameof(this.IdentityName));
            }

            if (string.IsNullOrEmpty(this.DeltaIdentityName))
            {
                throw new ArgumentNullException(nameof(this.DeltaIdentityName));
            }

            if (string.IsNullOrEmpty(this.BaselineIdentityName))
            {
                throw new ArgumentNullException(nameof(this.BaselineIdentityName));
            }

            // All three are addressed independently by a client, so any two of them sharing a name
            // would make the source untestable rather than merely unrealistic.
            var identities = new HashSet<string>(
                new[] { this.IdentityName, this.DeltaIdentityName, this.BaselineIdentityName },
                StringComparer.Ordinal);

            if (identities.Count != 3)
            {
                throw new ArgumentException("The full index, the delta and the baseline must each have an identity of their own", nameof(this.IdentityName));
            }

            if (string.IsNullOrEmpty(this.Version))
            {
                throw new ArgumentNullException(nameof(this.Version));
            }

            if (string.IsNullOrEmpty(this.BaselineVersion))
            {
                throw new ArgumentNullException(nameof(this.BaselineVersion));
            }

            if (string.IsNullOrEmpty(this.BaselineRelativePath))
            {
                throw new ArgumentNullException(nameof(this.BaselineRelativePath));
            }

            if (this.ManifestsNotInBaseline.Count == 0)
            {
                throw new ArgumentException("An empty delta cannot demonstrate anything", nameof(this.ManifestsNotInBaseline));
            }
        }
    }
}
