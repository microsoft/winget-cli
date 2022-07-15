// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using System.Collections.Generic;
    using System.Linq;

    public class BaseInterop
    {
        public WinGetProjectionFactory TestFactory { get; }

        public BaseInterop(IInstanceInitializer initializer)
        {
            TestFactory = new(initializer);
        }

        /// <summary>
        /// Find all filtered package from a provided package catalog reference.
        /// </summary>
        /// <param name="packageCatalogReference">Package catalog reference</param>
        /// <param name="field">Package match field</param>
        /// <param name="option">Package field match option</param>
        /// <param name="value">Package match value</param>
        /// <returns>List of matches</returns>
        protected IReadOnlyList<MatchResult> FindAllPackages(
            PackageCatalogReference packageCatalogReference,
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value)
        {
            Assert.NotNull(packageCatalogReference, "Package catalog reference cannot be null");

            // Prepare filter
            var filter = TestFactory.CreatePackageMatchFilter();
            filter.Field = field;
            filter.Option = option;
            filter.Value = value;

            // Add filter
            var findPackageOptions = TestFactory.CreateFindPackagesOptions();
            findPackageOptions.Filters.Add(filter);

            // Connect and find package
            var source = packageCatalogReference.Connect().PackageCatalog;
            return source.FindPackages(findPackageOptions).Matches;
        }

        /// <summary>
        /// Find one filtered package from a provided package catalog reference.
        /// </summary>
        /// <param name="packageCatalogReference">Package catalog reference</param>
        /// <param name="field">Package match field</param>
        /// <param name="option">Package field match option</param>
        /// <param name="value">Package match value</param>
        /// <returns>List of matches</returns>
        public MatchResult FindOnePackage(
            PackageCatalogReference packageCatalogReference,
            PackageMatchField field,
            PackageFieldMatchOption option,
            string value)
        {
            var findPackages = FindAllPackages(packageCatalogReference, field, option, value);
            Assert.AreEqual(1, findPackages.Count, $"Expected exactly one package but found {findPackages.Count}");
            return findPackages.First();
        }
    }
}
