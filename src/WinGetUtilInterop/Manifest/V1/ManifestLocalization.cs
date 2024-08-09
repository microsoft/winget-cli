// -----------------------------------------------------------------------------
// <copyright file="ManifestLocalization.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;

    /// <summary>
    /// Class that contains the elements that represent what a manifest localization element can contain in the manifest file.
    /// </summary>
    public class ManifestLocalization
    {
        /// <summary>
        /// Gets or sets the locale.
        /// </summary>
        public string PackageLocale { get; set; }

        /// <summary>
        /// Gets or sets the publisher.
        /// </summary>
        public string Publisher { get; set; }

        /// <summary>
        /// Gets or sets the publisher url.
        /// </summary>
        public string PublisherUrl { get; set; }

        /// <summary>
        /// Gets or sets the publisher support url.
        /// </summary>
        public string PublisherSupportUrl { get; set; }

        /// <summary>
        /// Gets or sets the default privacy url.
        /// </summary>
        public string PrivacyUrl { get; set; }

        /// <summary>
        /// Gets or sets the author.
        /// </summary>
        public string Author { get; set; }

        /// <summary>
        /// Gets or sets the package name.
        /// </summary>
        public string PackageName { get; set; }

        /// <summary>
        /// Gets or sets the package homepage url.
        /// </summary>
        public string PackageUrl { get; set; }

        /// <summary>
        /// Gets or sets the license.
        /// </summary>
        public string License { get; set; }

        /// <summary>
        /// Gets or sets the license url.
        /// </summary>
        public string LicenseUrl { get; set; }

        /// <summary>
        /// Gets or sets the copyright.
        /// </summary>
        public string Copyright { get; set; }

        /// <summary>
        /// Gets or sets the copyright url.
        /// </summary>
        public string CopyrightUrl { get; set; }

        /// <summary>
        /// Gets or sets the package short description.
        /// </summary>
        public string ShortDescription { get; set; }

        /// <summary>
        /// Gets or sets the package description.
        /// </summary>
        public string Description { get; set; }

        /// <summary>
        /// Gets or sets the Tags.
        /// </summary>
        public List<string> Tags { get; set; }

        /// <summary>
        /// Gets or sets the package agreements.
        /// </summary>
        public List<PackageAgreement> Agreements { get; set; }

        /// <summary>
        /// Gets or sets the release notes.
        /// </summary>
        public string ReleaseNotes { get; set; }

        /// <summary>
        /// Gets or sets the release notes url.
        /// </summary>
        public string ReleaseNotesUrl { get; set; }

        /// <summary>
        /// Gets or sets the purchase url of the package.
        /// </summary>
        public string PurchaseUrl { get; set; }

        /// <summary>
        /// Gets or sets the installation notes.
        /// </summary>
        public string InstallationNotes { get; set; }

        /// <summary>
        /// Gets or sets the manifest documentation.
        /// </summary>
        public List<ManifestDocumentation> Documentations { get; set; }

        /// <summary>
        /// Gets or sets the manifest icons information.
        /// </summary>
        public List<ManifestIcon> Icons { get; set; }

        /// <summary>
        /// Returns a List of strings containing the URIs contained within this localization.
        /// </summary>
        /// <returns>List of strings.</returns>
        public List<string> GetURIs()
        {
            List<string> uris = new List<string>();
            if (!string.IsNullOrEmpty(this.PublisherUrl))
            {
                uris.Add(this.PublisherUrl);
            }

            if (!string.IsNullOrEmpty(this.PublisherSupportUrl))
            {
                uris.Add(this.PublisherSupportUrl);
            }

            if (!string.IsNullOrEmpty(this.PrivacyUrl))
            {
                uris.Add(this.PrivacyUrl);
            }

            if (!string.IsNullOrEmpty(this.PackageUrl))
            {
                uris.Add(this.PackageUrl);
            }

            if (!string.IsNullOrEmpty(this.LicenseUrl))
            {
                uris.Add(this.LicenseUrl);
            }

            if (!string.IsNullOrEmpty(this.CopyrightUrl))
            {
                uris.Add(this.CopyrightUrl);
            }

            if (!string.IsNullOrEmpty(this.ReleaseNotesUrl))
            {
                uris.Add(this.ReleaseNotesUrl);
            }

            if (this.Agreements != null)
            {
                foreach (var agreement in this.Agreements)
                {
                    if (!string.IsNullOrEmpty(agreement.AgreementUrl))
                    {
                        uris.Add(agreement.AgreementUrl);
                    }
                }
            }

            if (this.Documentations != null)
            {
                foreach (var docs in this.Documentations)
                {
                    if (!string.IsNullOrEmpty(docs.DocumentUrl))
                    {
                        uris.Add(docs.DocumentUrl);
                    }
                }
            }

            if (this.Icons != null)
            {
                foreach (var icon in this.Icons)
                {
                    if (!string.IsNullOrEmpty(icon.IconUrl))
                    {
                        uris.Add(icon.IconUrl);
                    }
                }
            }

            return uris;
        }
    }
}
