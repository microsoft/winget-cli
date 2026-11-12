// -----------------------------------------------------------------------------
// <copyright file="FinderCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using System.Reflection;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Attributes;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// This is the base class for all commands that might need to search for a package. It contains an initial
    /// set of parameters that corresponds to the intersection of i.e., the "install" and "search" commands.
    /// </summary>
    public abstract class FinderCommand : ManagementDeploymentCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FinderCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal FinderCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Gets or sets a value indicating whether the user accepted the agreements of all sources.
        /// </summary>
        public bool AcceptSourceAgreements { get; set; }

        /// <summary>
        /// Gets or sets the field that is matched against the identifier of a package.
        /// </summary>
        [Filter(Field = PackageMatchField.Id)]
        protected string? Id { get; set; }

        /// <summary>
        /// Gets or sets the field that is matched against the name of a package.
        /// </summary>
        [Filter(Field = PackageMatchField.Name)]
        protected string? Name { get; set; }

        /// <summary>
        /// Gets or sets the field that is matched against the moniker of a package.
        /// </summary>
        [Filter(Field = PackageMatchField.Moniker)]
        protected string? Moniker { get; set; }

        /// <summary>
        /// Gets or sets the name of the source to search for packages. If null, then all sources are searched.
        /// </summary>
        protected string? Source { get; set; }

        /// <summary>
        /// Gets or sets how to match against package fields.
        /// </summary>
#pragma warning disable SA1011 // Closing square brackets should be spaced correctly
        protected string[]? Query { get; set; }
#pragma warning restore SA1011 // Closing square brackets should be spaced correctly

        private string? QueryAsJoinedString
        {
            get
            {
                return this.Query is null
                    ? null
                    : string.Join(" ", this.Query);
            }
        }

        /// <summary>
        /// Searches for packages based on the configured parameters.
        /// </summary>
        /// <param name="behavior">The <see cref="CompositeSearchBehavior" /> value.</param>
        /// <param name="limit">The limit on the number of matches returned.</param>
        /// <param name="match">The match option.</param>
        /// <returns>A list of <see cref="MatchResult" /> objects.</returns>
        internal IReadOnlyList<MatchResult> FindPackages(
            CompositeSearchBehavior behavior,
            uint limit,
            PackageFieldMatchOption match)
        {
            PackageCatalog catalog = this.GetPackageCatalog(behavior);
            FindPackagesOptions options = this.GetFindPackagesOptions(limit, match);
            return this.GetMatchResults(catalog, options);
        }

        /// <summary>
        /// Sets the find package options for a query input.
        /// DO NOT pass PackageFieldMatchOption WinRT enum type in this method.
        /// That will cause the type to attempt to be loaded in the construction
        /// of this method and throw a different exception for Windows PowerShell.
        /// </summary>
        /// <param name="options">The options object.</param>
        /// <param name="match">The match type as string.</param>
        /// <param name="value">The query value.</param>
        internal virtual void SetQueryInFindPackagesOptions(
            ref FindPackagesOptions options,
            string match,
            string? value)
        {
            var selector = ManagementDeploymentFactory.Instance.CreatePackageMatchFilter();
            selector.Field = PackageMatchField.CatalogDefault;
            selector.Value = value ?? string.Empty;
            selector.Option = PSEnumHelpers.ToPackageFieldMatchOption(match);
            options.Selectors.Add(selector);
        }

        private void AddFilterToFindPackagesOptionsIfNotNull(
            ref FindPackagesOptions options,
            PackageMatchField field,
            PackageFieldMatchOption match,
            string? value)
        {
            if (value != null)
            {
                var filter = ManagementDeploymentFactory.Instance.CreatePackageMatchFilter();
                filter.Field = field;
                filter.Value = value;
                filter.Option = match;
                options.Filters.Add(filter);
            }
        }

        private IReadOnlyList<MatchResult> GetMatchResults(
            PackageCatalog catalog,
            FindPackagesOptions options)
        {
            FindPackagesResult result = catalog.FindPackages(options);
            if (result.Status == FindPackagesResultStatus.Ok)
            {
                return result.Matches;
            }
            else
            {
                throw new FindPackagesException(result.Status);
            }
        }

        private PackageCatalog GetPackageCatalog(CompositeSearchBehavior behavior)
        {
            PackageCatalogReference reference = this.GetPackageCatalogReference(behavior);
            ConnectResult result = reference.Connect();
            if (result.Status == ConnectResultStatus.Ok)
            {
                return result.PackageCatalog;
            }
            else
            {
                throw new CatalogConnectException(result.ExtendedErrorCode);
            }
        }

        private PackageCatalogReference GetPackageCatalogReference(CompositeSearchBehavior behavior)
        {
            CreateCompositePackageCatalogOptions options = ManagementDeploymentFactory.Instance.CreateCreateCompositePackageCatalogOptions();
            IReadOnlyList<PackageCatalogReference> references = this.GetPackageCatalogReferences(this.Source);
            bool anyExcluded = false;
            for (var i = 0; i < references.Count; i++)
            {
                var reference = references[i];
                bool isExplicit = false;
                try
                {
                    // Execute in try block to catch interface not implemented on older servers.
                    isExplicit = reference.Info.Explicit;
                }
                catch
                {
                    // Assume that any failure other than the interface not implemented to get Explicit
                    // will result in other failures shortly after this (like the server being gone).
                }

                if (!isExplicit)
                {
                    if (this.ObtainSourceAgreementsConsent(reference))
                    {
                        options.Catalogs.Add(reference);
                    }
                    else
                    {
                        anyExcluded = true;
                    }
                }
            }

            // Searches of installed packages can continue without any remote source.
            if (anyExcluded && options.Catalogs.Count == 0 && behavior != CompositeSearchBehavior.LocalCatalogs)
            {
                throw new NoSourcesAvailableException();
            }

            options.CompositeSearchBehavior = behavior;
            return PackageManagerWrapper.Instance.CreateCompositePackageCatalog(options);
        }

        /// <summary>
        /// Obtains consent to the agreements of a source.
        /// </summary>
        /// <param name="reference">The source.</param>
        /// <returns>True if the source can be used; false if it must be excluded.</returns>
        private bool ObtainSourceAgreementsConsent(PackageCatalogReference reference)
        {
            string sourceName = reference.Info.Name;
            string? agreements;

            try
            {
                if (reference.SourceAgreementsSatisfied)
                {
                    return true;
                }

                agreements = AgreementConsent.FormatSourceAgreements(reference, includeImplicitFields: true);
            }
            catch (Exception e) when (AgreementConsent.IsMissingContract(e))
            {
                // The server cannot report whether the agreements were accepted, so keep its behavior of accepting them.
                string? legacyAgreements = null;
                try
                {
                    legacyAgreements = AgreementConsent.FormatSourceAgreements(reference, includeImplicitFields: false);
                }
                catch (Exception legacyException)
                {
                    this.Write(StreamType.Warning, string.Format(Resources.SourceAgreementsUnavailableUnsupported, sourceName, legacyException.Message));
                }

                if (legacyAgreements != null)
                {
                    this.Write(StreamType.Warning, legacyAgreements + Environment.NewLine + string.Format(Resources.AgreementsAcceptedUnsupported, AgreementConsent.AcceptSourceAgreementsParameter));
                }

                return true;
            }
            catch (Exception e)
            {
                this.Write(StreamType.Warning, string.Format(Resources.SourceAgreementsUnavailable, sourceName, e.Message));
                return false;
            }

            // The agreements are not satisfied, but there are none to show; the source removed them all.
            if (agreements == null)
            {
                return true;
            }

            AgreementConsent.Decision decision = AgreementConsent.Obtain(
                this,
                agreements,
                Resources.SourceAgreementsPrompt,
                Resources.SourceAgreementsCaption,
                this.AcceptSourceAgreements,
                AgreementConsent.AcceptSourceAgreementsParameter);

            switch (decision)
            {
                case AgreementConsent.Decision.Accepted:
                    try
                    {
                        reference.SaveAcceptedSourceAgreements();
                    }
                    catch
                    {
                        // Failing to record acceptance only means that the user will be asked again.
                    }

                    return true;
                case AgreementConsent.Decision.AcceptedWithoutPrompt:
                    return true;
                default:
                    this.Write(StreamType.Warning, string.Format(Resources.SourceAgreementsDeclined, sourceName));
                    return false;
            }
        }

        private FindPackagesOptions GetFindPackagesOptions(uint limit, PackageFieldMatchOption match)
        {
            var options = ManagementDeploymentFactory.Instance.CreateFindPackagesOptions();
            this.SetQueryInFindPackagesOptions(ref options, match.ToString(), this.QueryAsJoinedString);
            this.AddAttributedFiltersToFindPackagesOptions(ref options, match);
            options.ResultLimit = limit;
            return options;
        }

        private void AddAttributedFiltersToFindPackagesOptions(
            ref FindPackagesOptions options,
            PackageFieldMatchOption match)
        {
            IEnumerable<PropertyInfo> properties = this.GetType()
                .GetProperties(BindingFlags.NonPublic | BindingFlags.Instance)
                .Where(property => Attribute.IsDefined(property, typeof(FilterAttribute)));

            foreach (PropertyInfo info in properties)
            {
                if (info.GetCustomAttribute(typeof(FilterAttribute), true) is FilterAttribute attribute)
                {
                    PackageMatchField field = attribute.Field;
                    string? value = info.GetValue(this, null) as string;
                    this.AddFilterToFindPackagesOptionsIfNotNull(ref options, field, match, value);
                }
            }
        }
    }
}
