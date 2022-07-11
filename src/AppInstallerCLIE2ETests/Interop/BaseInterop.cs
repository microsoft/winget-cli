namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;
    using System;
    using System.Collections.Generic;
    using System.Linq;

    public class BaseInterop : BaseCommand
    {
        public WinGetProjectionFactory TestFactory { get; }

        public BaseInterop(IInstanceInitializer initializer)
        {
            TestFactory = new(initializer);
        }

        /// <summary>
        /// Force garbage collection after each test class execution.
        /// Note: Connecting to a package catalog reference without calling the
        ///       garbage collector at the end of the test class, causes subsequent
        ///       test classes to freeze and timeout when attempting to reset then
        ///       add the TestSource to winget from the command line during the setup
        ///       phase.
        /// </summary>
        [OneTimeTearDown]
        public void GarbageCollection()
        {
            GC.Collect();
            GC.WaitForPendingFinalizers();
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
