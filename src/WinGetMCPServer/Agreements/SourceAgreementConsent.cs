// -----------------------------------------------------------------------------
// <copyright file="SourceAgreementConsent.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Agreements
{
    using System.Collections.Concurrent;
    using Microsoft.Management.Deployment;
    using ModelContextProtocol.Server;
    using WinGetMCPServer.Response;

    /// <summary>
    /// The catalogs to connect after obtaining consent to their source agreements.
    /// </summary>
    internal sealed class CatalogSelection
    {
        /// <summary>
        /// Gets the catalogs that may be used.
        /// </summary>
        public List<PackageCatalogReference> Included { get; } = new List<PackageCatalogReference>();

        /// <summary>
        /// Gets the catalogs that were excluded.
        /// </summary>
        public ExcludedSources Excluded { get; } = new ExcludedSources();

        /// <summary>
        /// Gets the agreements that must be presented to the user by the caller before continuing.
        /// </summary>
        public List<AgreementInfo> Pending { get; } = new List<AgreementInfo>();
    }

    /// <summary>
    /// Obtains consent to the agreements of package sources before they are used.
    /// </summary>
    internal static class SourceAgreementConsent
    {
        /// <summary>
        /// The sources whose agreements the user declined in this server session. These are not asked
        /// about again unless the caller indicates that the user has changed their mind. Tools are
        /// constructed per call, so this state must be static; it is never persisted.
        /// </summary>
        private static readonly ConcurrentDictionary<string, byte> DeclinedSources = new ConcurrentDictionary<string, byte>(StringComparer.OrdinalIgnoreCase);

        /// <summary>
        /// Determines which catalogs may be used, obtaining consent to their agreements where required.
        /// </summary>
        /// <param name="server">The server.</param>
        /// <param name="catalogs">The individual (non-composite) catalogs to consider.</param>
        /// <param name="acceptAgreements">The decision provided by the caller, if any.</param>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>The catalogs to use.</returns>
        public static async Task<CatalogSelection> SelectAsync(
            McpServer server,
            IEnumerable<PackageCatalogReference> catalogs,
            bool? acceptAgreements,
            CancellationToken cancellationToken)
        {
            CatalogSelection selection = new CatalogSelection();

            foreach (PackageCatalogReference catalog in catalogs)
            {
                cancellationToken.ThrowIfCancellationRequested();

                string name = catalog.Info.Name;
                List<AgreementInfo> agreements;

                try
                {
                    if (catalog.SourceAgreementsSatisfied)
                    {
                        selection.Included.Add(catalog);
                        continue;
                    }

                    agreements = GetAgreements(catalog, name);
                }
                catch (Exception)
                {
                    // Reading the agreements requires contacting the source. If that fails, the catalog cannot
                    // be used: connecting would retry the request and, should it then succeed, use the source
                    // without the user having seen its agreements.
                    selection.Excluded.Unavailable.Add(name);
                    continue;
                }

                if (acceptAgreements != true && DeclinedSources.ContainsKey(name))
                {
                    selection.Excluded.NotAccepted.Add(name);
                    continue;
                }

                ConsentDecision decision = await AgreementConsent.ObtainAsync(
                    server,
                    LocalizedStrings.Format("SourceAgreementsTitle", name),
                    agreements,
                    LocalizedStrings.Get("SourceAgreementsPrompt"),
                    acceptAgreements,
                    cancellationToken);

                switch (decision)
                {
                    case ConsentDecision.Accepted:
                        DeclinedSources.TryRemove(name, out _);
                        try
                        {
                            catalog.SaveAcceptedSourceAgreements();
                        }
                        catch (Exception)
                        {
                            // The user has consented, so the catalog can be used; the user will just be
                            // asked again in the future.
                        }

                        selection.Included.Add(catalog);
                        break;
                    case ConsentDecision.Declined:
                        DeclinedSources.TryAdd(name, 0);
                        selection.Excluded.NotAccepted.Add(name);
                        break;
                    case ConsentDecision.Dismissed:
                        selection.Excluded.NotAccepted.Add(name);
                        break;
                    case ConsentDecision.Pending:
                        selection.Pending.AddRange(agreements);
                        break;
                }
            }

            return selection;
        }

        private static List<AgreementInfo> GetAgreements(PackageCatalogReference catalog, string name)
        {
            List<AgreementInfo> result = new List<AgreementInfo>();

            // Enumerating WinRT collections through IEnumerable fails with the embedded projection, so index instead.
            var sourceAgreements = catalog.SourceAgreements;
            for (int i = 0; i < sourceAgreements.Count; ++i)
            {
                SourceAgreement agreement = sourceAgreements[i];
                result.Add(new AgreementInfo()
                {
                    Kind = AgreementInfo.SourceKind,
                    Source = name,
                    Label = NullIfEmpty(agreement.Label),
                    Text = NullIfEmpty(agreement.Text),
                    Url = NullIfEmpty(agreement.Url),
                });
            }

            if ((catalog.ImplicitAgreementFields & ImplicitAgreementFields.Market) != 0)
            {
                // The CLI presents this disclosure without a label, so none is given here either.
                result.Add(new AgreementInfo()
                {
                    Kind = AgreementInfo.SourceKind,
                    Source = name,
                    Text = LocalizedStrings.Get("SourceAgreementsMarketMessage"),
                });
            }

            return result;
        }

        private static string? NullIfEmpty(string? value)
        {
            return string.IsNullOrEmpty(value) ? null : value;
        }
    }
}
