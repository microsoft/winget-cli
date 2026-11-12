// -----------------------------------------------------------------------------
// <copyright file="PackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using System.Threading.Tasks;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Extensions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// This is the base class for commands which operate on a specific package and version i.e.,
    /// the "install", "uninstall", "download", and "upgrade" commands.
    /// </summary>
    public abstract class PackageCommand : FinderCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PackageCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal PackageCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Gets or sets a value indicating whether the user accepted the agreements of the package.
        /// </summary>
        public bool AcceptPackageAgreements { get; set; }

        /// <summary>
        /// Gets or sets the package to directly install.
        /// </summary>
        /// <remarks>
        /// Must match the name of the <see cref="CatalogPackage" /> field on the <see cref="MatchResult" /> class.
        /// </remarks>
        protected PSCatalogPackage? CatalogPackage { get; set; } = null;

        /// <summary>
        /// Gets or sets the version to install.
        /// </summary>
        protected string? Version { get; set; }

        /// <summary>
        /// Executes a command targeting a specific package version.
        /// </summary>
        /// <typeparam name="TResult">Type of callback's result.</typeparam>
        /// <param name="behavior">The <see cref="CompositeSearchBehavior" /> value.</param>
        /// <param name="match">The match option.</param>
        /// <param name="callback">The method to call after retrieving the package and version to operate upon.</param>
        /// <returns>Result of the callback.</returns>
        internal async Task<Tuple<TResult, CatalogPackage>?> GetPackageAndExecuteAsync<TResult>(
            CompositeSearchBehavior behavior,
            PackageFieldMatchOption match,
            Func<CatalogPackage, PackageVersionId?, Task<TResult>> callback)
            where TResult : class
        {
            CatalogPackage package = this.GetCatalogPackage(behavior, match);
            PackageVersionId? version = this.GetPackageVersionId(package);
            if (this.ShouldProcess(package.ToString(version)))
            {
                var result = await callback(package, version);
                return new Tuple<TResult, CatalogPackage>(result, package);
            }

            return null;
        }

        /// <summary>
        /// Obtains consent to the agreements of a package version, throwing if the user declines them.
        /// </summary>
        /// <param name="package">The package.</param>
        /// <param name="versionInfo">The version whose agreements apply; if null, there are none to obtain.</param>
        /// <param name="locale">The preferred locale of the agreements.</param>
        internal void ObtainPackageAgreementsConsent(CatalogPackage package, PackageVersionInfo? versionInfo, string? locale = null)
        {
            if (versionInfo == null)
            {
                // The operation itself reports that there is no version to use.
                return;
            }

            string description = $"{package.Name} [{package.Id}] Version {versionInfo.Version}";
            string? agreements;

            try
            {
                CatalogPackageMetadata metadata = string.IsNullOrEmpty(locale)
                    ? versionInfo.GetCatalogPackageMetadata()
                    : versionInfo.GetCatalogPackageMetadata(locale);
                agreements = AgreementConsent.FormatPackageAgreements(description, metadata?.Agreements);
            }
            catch (Exception e)
            {
                throw new PackageAgreementsUnavailableException(description, e);
            }

            if (agreements == null)
            {
                return;
            }

            AgreementConsent.Decision decision = AgreementConsent.Obtain(
                this,
                agreements,
                Resources.PackageAgreementsPrompt,
                Resources.PackageAgreementsCaption,
                this.AcceptPackageAgreements,
                AgreementConsent.AcceptPackageAgreementsParameter);

            if (decision == AgreementConsent.Decision.Declined)
            {
                throw new PackageAgreementsNotAcceptedException(description);
            }
        }

        /// <summary>
        /// Sets the find package options for a query input that is looking for a specific package.
        /// DO NOT pass PackageFieldMatchOption WinRT enum type in this method.
        /// That will cause the type to attempt to be loaded in the construction
        /// of this method and throw a different exception for Windows PowerShell.
        /// </summary>
        /// <param name="options">The options object.</param>
        /// <param name="match">The match type.</param>
        /// <param name="value">The query value.</param>
        internal override void SetQueryInFindPackagesOptions(
            ref FindPackagesOptions options,
            string match,
            string? value)
        {
            var matchOption = PSEnumHelpers.ToPackageFieldMatchOption(match);
            foreach (PackageMatchField field in new PackageMatchField[] { PackageMatchField.Id, PackageMatchField.Name, PackageMatchField.Moniker })
            {
                var selector = ManagementDeploymentFactory.Instance.CreatePackageMatchFilter();
                selector.Field = field;
                selector.Value = value ?? string.Empty;
                selector.Option = matchOption;
                options.Selectors.Add(selector);
            }
        }

        private CatalogPackage GetCatalogPackage(CompositeSearchBehavior behavior, PackageFieldMatchOption match)
        {
            if (this.CatalogPackage != null)
            {
                // The package was already provided via a parameter or the pipeline.
                return this.CatalogPackage.CatalogPackageCOM;
            }
            else
            {
                IReadOnlyList<MatchResult> results = this.FindPackages(behavior, 0, match);
                if (results.Count == 1)
                {
                    // Exactly one package matched, so we can just return it.
                    return results[0].CatalogPackage;
                }
                else if (results.Count == 0)
                {
                    // No packages matched, we need to throw an error.
                    throw new NoPackageFoundException();
                }
                else
                {
                    if (behavior != CompositeSearchBehavior.LocalCatalogs)
                    {
                        List<MatchResult> highestPriorityResults = new List<MatchResult>();
                        int? highestPriority = null;

                        for (int i = 0; i < results.Count; i++)
                        {
                            MatchResult result = results[i];
                            int? priority = result.CatalogPackage.CatalogPriority;

                            if ((highestPriority == null && priority != null) || highestPriority < priority)
                            {
                                // Current priority is higher; reset.
                                highestPriority = priority;
                                highestPriorityResults.Clear();
                            }
                            else if (highestPriority == priority)
                            {
                                // Priority is equal, add to the list.
                            }
                            else
                            {
                                // Current priority is lower, ignore the match.
                                continue;
                            }

                            highestPriorityResults.Add(result);
                        }

                        if (highestPriorityResults.Count == 1)
                        {
                            return highestPriorityResults[0].CatalogPackage;
                        }
                        else
                        {
                            throw new VagueCriteriaException(highestPriorityResults);
                        }
                    }

                    // Too many packages matched! The user needs to refine their input.
                    throw new VagueCriteriaException(results);
                }
            }
        }

        private PackageVersionId? GetPackageVersionId(CatalogPackage package)
        {
            if (this.Version != null)
            {
                for (var i = 0; i < package.AvailableVersions.Count; i++)
                {
                    PackageVersionInfo versionInfo = package.GetPackageVersionInfo(package.AvailableVersions[i]);

                    if (versionInfo != null && versionInfo.CompareToVersion(this.Version) == CompareResult.Equal)
                    {
                        return package.AvailableVersions[i];
                    }
                }

                throw new InvalidVersionException(this.Version);
            }
            else
            {
                return null;
            }
        }
    }
}
