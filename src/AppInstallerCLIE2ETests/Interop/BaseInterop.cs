// -----------------------------------------------------------------------------
// <copyright file="BaseInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Base interop class.
    /// </summary>
    public abstract class BaseInterop
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BaseInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public BaseInterop(IInstanceInitializer initializer)
        {
            this.TestFactory = new (initializer);
        }

        /// <summary>
        /// Gets the test factory.
        /// </summary>
        public WinGetProjectionFactory TestFactory { get; }

        /// <summary>
        /// Find one filtered package from a provided package catalog reference.
        /// </summary>
        /// <param name="packageCatalogReference">Package catalog reference.</param>
        /// <param name="field">Package match field.</param>
        /// <param name="option">Package field match option.</param>
        /// <param name="value">Package match value.</param>
        /// <returns>List of matches.</returns>
        public MatchResult FindOnePackage(
            PackageCatalogReference packageCatalogReference,
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value)
        {
            var findPackages = this.FindAllPackages(packageCatalogReference, field, option, value);
            Assert.AreEqual(1, findPackages.Count, $"Expected exactly one package but found {findPackages.Count}");
            return findPackages.First();
        }

        /// <summary>
        /// Find all filtered package from a provided package catalog reference.
        /// </summary>
        /// <param name="packageCatalogReference">Package catalog reference.</param>
        /// <param name="field">Package match field.</param>
        /// <param name="option">Package field match option.</param>
        /// <param name="value">Package match value.</param>
        /// <returns>List of matches.</returns>
        protected IReadOnlyList<MatchResult> FindAllPackages(
            PackageCatalogReference packageCatalogReference,
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value)
        {
            Assert.NotNull(packageCatalogReference, "Package catalog reference cannot be null");

            // Prepare filter
            var filter = this.TestFactory.CreatePackageMatchFilter();
            filter.Field = field;
            filter.Option = option;
            filter.Value = value;

            // Add filter
            var findPackageOptions = this.TestFactory.CreateFindPackagesOptions();
            findPackageOptions.Filters.Add(filter);

            // Connect and find package
            var source = packageCatalogReference.Connect().PackageCatalog;

            return source.FindPackages(findPackageOptions).Matches;
        }
    }
}
