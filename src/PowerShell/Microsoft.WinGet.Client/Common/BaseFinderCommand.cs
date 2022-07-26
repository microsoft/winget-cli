// -----------------------------------------------------------------------------
// <copyright file="BaseFinderCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using System.Reflection;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Attributes;
    using Microsoft.WinGet.Client.Errors;

    /// <summary>
    /// This is the base class for all commands that might need to search for a package. It contains an initial
    /// set of parameters that corresponds to the intersection of i.e., the "install" and "search" commands.
    /// </summary>
    public class BaseFinderCommand : BaseClientCommand
    {
        /// <summary>
        /// Gets or sets the field that is matched against the identifier of a package.
        /// </summary>
        [Filter(Field = PackageMatchField.Id)]
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Id { get; set; }

        /// <summary>
        /// Gets or sets the field that is matched against the name of a package.
        /// </summary>
        [Filter(Field = PackageMatchField.Name)]
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the field that is matched against the name of a package.
        /// </summary>
        [Filter(Field = PackageMatchField.Moniker)]
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Moniker { get; set; }

        /// <summary>
        /// Gets or sets the name of the source to search for packages. If null, then all sources are searched.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets the strings that match against every field of a package.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            Position = 0,
            ValueFromPipelineByPropertyName = true,
            ValueFromRemainingArguments = true)]
        public string[] Query { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to match exactly against package fields.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Exact { get; set; }

        private string QueryAsJoinedString
        {
            get
            {
                return (this.Query is null)
                    ? null
                    : string.Join(" ", this.Query);
            }
        }

        /// <summary>
        /// Returns a <see cref="PackageFieldMatchOption" /> based on a parameter.
        /// </summary>
        /// <returns>A <see cref="PackageFieldMatchOption" /> value.</returns>
        protected virtual PackageFieldMatchOption GetExactAsMatchOption()
        {
            return this.Exact.ToBool()
                ? PackageFieldMatchOption.Equals
                : PackageFieldMatchOption.ContainsCaseInsensitive;
        }

        /// <summary>
        /// Searches for packages based on the configured parameters.
        /// </summary>
        /// <param name="behavior">The <see cref="CompositeSearchBehavior" /> value.</param>
        /// <param name="limit">The limit on the number of matches returned.</param>
        /// <returns>A list of <see cref="MatchResult" /> objects.</returns>
        protected IReadOnlyList<MatchResult> FindPackages(
            CompositeSearchBehavior behavior,
            uint limit)
        {
            PackageCatalog catalog = this.GetPackageCatalog(behavior);
            FindPackagesOptions options = this.GetFindPackagesOptions(limit);
            return GetMatchResults(catalog, options);
        }

        private static void SetQueryInFindPackagesOptions(
            ref FindPackagesOptions options,
            PackageFieldMatchOption match,
            string value)
        {
            var selector = ComObjectFactory.Value.CreatePackageMatchFilter();
            selector.Field = PackageMatchField.CatalogDefault;
            selector.Value = value ?? string.Empty;
            selector.Option = match;
            options.Selectors.Add(selector);
        }

        private static void AddFilterToFindPackagesOptionsIfNotNull(
            ref FindPackagesOptions options,
            PackageMatchField field,
            PackageFieldMatchOption match,
            string value)
        {
            if (value != null)
            {
                var filter = ComObjectFactory.Value.CreatePackageMatchFilter();
                filter.Field = field;
                filter.Value = value;
                filter.Option = match;
                options.Filters.Add(filter);
            }
        }

        private static IReadOnlyList<MatchResult> GetMatchResults(
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
                throw new RuntimeException(Utilities.ResourceManager.GetString("RuntimeExceptionCatalogError"));
            }
        }

        private PackageCatalogReference GetPackageCatalogReference(CompositeSearchBehavior behavior)
        {
            CreateCompositePackageCatalogOptions options = ComObjectFactory.Value.CreateCreateCompositePackageCatalogOptions();
            IReadOnlyList<PackageCatalogReference> references = GetPackageCatalogReferences(this.Source);
            for (var i = 0; i < references.Count; i++)
            {
                options.Catalogs.Add(references[i]);
            }

            options.CompositeSearchBehavior = behavior;
            return PackageManager.Value.CreateCompositePackageCatalog(options);
        }

        private FindPackagesOptions GetFindPackagesOptions(uint limit)
        {
            var options = ComObjectFactory.Value.CreateFindPackagesOptions();
            SetQueryInFindPackagesOptions(ref options, this.GetExactAsMatchOption(), this.QueryAsJoinedString);
            this.AddAttributedFiltersToFindPackagesOptions(ref options, this.GetExactAsMatchOption());
            options.ResultLimit = limit;
            return options;
        }

        private void AddAttributedFiltersToFindPackagesOptions(
            ref FindPackagesOptions options,
            PackageFieldMatchOption match)
        {
            IEnumerable<PropertyInfo> properties = this
                .GetType()
                .GetProperties()
                .Where(property => Attribute.IsDefined(property, typeof(FilterAttribute)));

            foreach (PropertyInfo info in properties)
            {
                if (info.GetCustomAttribute(typeof(FilterAttribute), true) is FilterAttribute attribute)
                {
                    PackageMatchField field = attribute.Field;
                    string value = info.GetValue(this, null) as string;
                    AddFilterToFindPackagesOptionsIfNotNull(ref options, field, match, value);
                }
            }
        }
    }
}
