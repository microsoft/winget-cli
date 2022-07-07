// -----------------------------------------------------------------------------
// <copyright file="BaseFinderCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using System.Reflection;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Attributes;
    using Microsoft.WinGet.Client.Errors;
    using Microsoft.WinGet.Client.Helpers;

    /// <summary>
    /// This is the base class for all commands that might need to search for packages.
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
        /// Gets or sets a value indicataing whether to match exactly against package fields.
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

        private PackageFieldMatchOption ExactAsMatchOption
        {
            get
            {
                return this.Exact.ToBool()
                    ? PackageFieldMatchOption.EqualsCaseInsensitive
                    : PackageFieldMatchOption.ContainsCaseInsensitive;
            }
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
            var catalog = this.GetPackageCatalog(behavior);
            var options = this.GetFindPackagesOptions(limit);
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
            var result = catalog.FindPackages(options);
            if (result.Status != FindPackagesResultStatus.Ok)
            {
                throw new FindPackagesException(result.Status);
            }

            return result.Matches;
        }

        private PackageCatalog GetPackageCatalog(CompositeSearchBehavior behavior)
        {
            var reference = this.GetPackageCatalogReference(behavior);
            var result = reference.Connect();
            if (result.Status != ConnectResultStatus.Ok)
            {
                throw new RuntimeException("There was an error connecting to the source.");
            }

            return result.PackageCatalog;
        }

        private PackageCatalogReference GetPackageCatalogReference(CompositeSearchBehavior behavior)
        {
            var options = ComObjectFactory.Value.CreateCreateCompositePackageCatalogOptions();
            var references = GetPackageCatalogReferences(this.Source);
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
            SetQueryInFindPackagesOptions(ref options, this.ExactAsMatchOption, this.QueryAsJoinedString);
            this.AddAttributedFiltersToFindPackagesOptions(ref options, this.ExactAsMatchOption);
            options.ResultLimit = limit;
            return options;
        }

        private void AddAttributedFiltersToFindPackagesOptions(
            ref FindPackagesOptions options,
            PackageFieldMatchOption match)
        {
            var properties = this
                .GetType()
                .GetProperties()
                .Where(property => Attribute.IsDefined(property, typeof(FilterAttribute)));

            foreach (var info in properties)
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
