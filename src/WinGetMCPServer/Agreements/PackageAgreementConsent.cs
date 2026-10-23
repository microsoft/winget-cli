// -----------------------------------------------------------------------------
// <copyright file="PackageAgreementConsent.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Agreements
{
    using Microsoft.Management.Deployment;
    using WinGetMCPServer.Response;

    /// <summary>
    /// Reads the agreements that a package requires to be accepted before installing.
    /// </summary>
    internal static class PackageAgreementConsent
    {
        /// <summary>
        /// Gets the agreements for the version of a package that would be installed.
        /// </summary>
        /// <param name="packageVersion">The package version that would be installed.</param>
        /// <returns>The agreements; empty if there are none or they cannot be read.</returns>
        public static List<AgreementInfo> GetAgreements(PackageVersionInfo? packageVersion)
        {
            List<AgreementInfo> result = new List<AgreementInfo>();

            if (packageVersion == null)
            {
                return result;
            }

            try
            {
                // Uses the localization selected by the user's WinGet settings.
                CatalogPackageMetadata? metadata = packageVersion.GetCatalogPackageMetadata();

                if (metadata?.Agreements == null)
                {
                    return result;
                }

                string? source = packageVersion.PackageCatalog?.Info?.Name;

                // Enumerating WinRT collections through IEnumerable fails with the embedded projection, so index instead.
                var agreements = metadata.Agreements;
                for (int i = 0; i < agreements.Count; ++i)
                {
                    PackageAgreement agreement = agreements[i];
                    result.Add(new AgreementInfo()
                    {
                        Kind = AgreementInfo.PackageKind,
                        Source = source,
                        Package = packageVersion.Id,
                        Label = NullIfEmpty(agreement.Label),
                        Text = NullIfEmpty(agreement.Text),
                        Url = NullIfEmpty(agreement.Url),
                    });
                }
            }
            catch (Exception)
            {
                // Not every source can supply metadata. If none is available, there is nothing to present,
                // and the install will report if agreements were in fact required.
                result.Clear();
            }

            return result;
        }

        /// <summary>
        /// Gets the text that introduces the agreements of a package to the user, in the same form as the CLI.
        /// </summary>
        /// <param name="packageVersion">The package version that would be installed.</param>
        /// <returns>The introduction.</returns>
        public static string GetIntroduction(PackageVersionInfo packageVersion)
        {
            return $"{LocalizedStrings.Get("ReportIdentityForAgreements")} {packageVersion.DisplayName} [{packageVersion.Id}] {LocalizedStrings.Get("ShowVersion")} {packageVersion.Version}";
        }

        /// <summary>
        /// Gets the question asked of the user after the agreements of a package.
        /// </summary>
        /// <returns>The question.</returns>
        public static string GetQuestion()
        {
            return LocalizedStrings.Get("PackageAgreementsPrompt");
        }

        private static string? NullIfEmpty(string? value)
        {
            return string.IsNullOrEmpty(value) ? null : value;
        }
    }
}
