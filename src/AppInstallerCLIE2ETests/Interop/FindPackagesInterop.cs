// -----------------------------------------------------------------------------
// <copyright file="FindPackagesInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Test find package interop.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class FindPackagesInterop : BaseInterop
    {
        private PackageManager packageManager;
        private PackageCatalogReference testSource;

        /// <summary>
        /// Initializes a new instance of the <see cref="FindPackagesInterop"/> class.
        /// </summary>
        /// <param name="initializer">Initializer.</param>
        public FindPackagesInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
            this.packageManager = this.TestFactory.CreatePackageManager();
            this.testSource = this.packageManager.GetPackageCatalogByName(Constants.TestSourceName);
        }

        /// <summary>
        /// Test find package. no package.
        /// </summary>
        [Test]
        public void FindPackageDoesNotExist()
        {
            // Find package
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "DoesNotExist");

            // Assert
            Assert.AreEqual(0, searchResult.Count);
        }

        /// <summary>
        /// Test find package with multiple match.
        /// </summary>
        [Test]
        public void FindPackagesMultipleMatchingQuery()
        {
            // Find package
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Name, PackageFieldMatchOption.Equals, "TestExeInstaller");

            // Assert
            Assert.AreEqual(2, searchResult.Count);
        }

        /// <summary>
        /// Test to find a package and verify the CatalogPackageMetadata COM output.
        /// </summary>
        [Test]
        public void FindPackagesVerifyDefaultLocaleFields()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.CatalogPackageMetadata");

            Assert.AreEqual(1, searchResult.Count);

            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var catalogPackageMetadata = catalogPackage.GetCatalogPackageMetadata(packageVersionId, string.Empty);

            Assert.AreEqual("testAuthor", catalogPackageMetadata.Author);
            Assert.AreEqual("AppInstallerTest", catalogPackageMetadata.Publisher);
            Assert.AreEqual("https://testPublisherUrl.com", catalogPackageMetadata.PublisherUrl);
            Assert.AreEqual("https://testPublisherSupportUrl.com", catalogPackageMetadata.PublisherSupportUrl);
            Assert.AreEqual("https://testPrivacyUrl.com", catalogPackageMetadata.PrivacyUrl);

            Assert.AreEqual("https://testPackageUrl.com", catalogPackageMetadata.PackageUrl);
            Assert.AreEqual("testLicense", catalogPackageMetadata.License);
            Assert.AreEqual("https://testLicenseUrl.com", catalogPackageMetadata.LicenseUrl);
            Assert.AreEqual("testCopyright", catalogPackageMetadata.Copyright);
            Assert.AreEqual("https://testCopyrightUrl.com", catalogPackageMetadata.CopyrightUrl);
            Assert.AreEqual("testDescription", catalogPackageMetadata.Description);
            Assert.AreEqual("testShortDescription", catalogPackageMetadata.ShortDescription);

            var tags = catalogPackageMetadata.Tags;
            Assert.AreEqual(2, tags.Count);
            Assert.AreEqual("tag1", tags[0]);
            Assert.AreEqual("tag2", tags[1]);
            Assert.AreEqual("testReleaseNotes", catalogPackageMetadata.ReleaseNotes);
            Assert.AreEqual("https://testReleaseNotes.net", catalogPackageMetadata.ReleaseNotesUrl);
            Assert.AreEqual("testInstallationNotes", catalogPackageMetadata.InstallationNotes);

            var packageAgreements = catalogPackageMetadata.Agreements;
            Assert.AreEqual(1, packageAgreements.Count);

            var agreement = packageAgreements[0];
            Assert.AreEqual("testAgreementLabel", agreement.Label);
            Assert.AreEqual("testAgreementText", agreement.Text);
            Assert.AreEqual("https://testAgreementUrl.net", agreement.Url);

            var documentations = catalogPackageMetadata.Documentations;
            Assert.AreEqual(1, documentations.Count);

            var documentation = documentations[0];
            Assert.AreEqual("testDocumentLabel", documentation.DocumentLabel);
            Assert.AreEqual("https://testDocumentUrl.com", documentation.DocumentUrl);
        }

        /// <summary>
        /// Verifies that an exception is thrown if the provided locale string is invalid.
        /// </summary>
        [Test]
        public void FindPackagesInvalidLocale()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.CatalogPackageMetadata");
            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            Assert.Throws<System.ArgumentException>(() => searchResult[0].CatalogPackage.GetCatalogPackageMetadata(packageVersionId, "badLocale"));
        }

        /// <summary>
        /// Verifies that the correct CatalogPackageMetadata is exposed when specifying a locale.
        /// </summary>
        [Test]
        public void FindPackagesGetCatalogPackageMetadataLocale()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.MultipleLocale");
            Assert.AreEqual(1, searchResult.Count);

            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var catalogPackageMetadata = catalogPackage.GetCatalogPackageMetadata(packageVersionId, "zh-CN");

            Assert.AreEqual("zh-CN", catalogPackageMetadata.Locale);
            Assert.AreEqual("localeLicense", catalogPackageMetadata.License);
            Assert.AreEqual("localePackageName", catalogPackageMetadata.PackageName);
            Assert.AreEqual("localePublisher", catalogPackageMetadata.Publisher);

            var tags = catalogPackageMetadata.Tags;
            Assert.AreEqual(2, tags.Count);
            Assert.AreEqual("tag1", tags[0]);
            Assert.AreEqual("tag2", tags[1]);

            var packageAgreements = catalogPackageMetadata.Agreements;
            Assert.AreEqual(1, packageAgreements.Count);

            var agreement = packageAgreements[0];
            Assert.AreEqual("localeAgreementLabel", agreement.Label);
            Assert.AreEqual("localeAgreement", agreement.Text);
            Assert.AreEqual("https://localeAgreementUrl.net", agreement.Url);

            var documentations = catalogPackageMetadata.Documentations;
            Assert.AreEqual(1, documentations.Count);

            var documentation = documentations[0];
            Assert.AreEqual("localeDocumentLabel", documentation.DocumentLabel);
            Assert.AreEqual("https://localeDocumentUrl.com", documentation.DocumentUrl);
        }

        /// <summary>
        /// Verifies that GetAllCatalogPackageMetadata() returns the correct number of CatalogPackageMetadata.
        /// </summary>
        [Test]
        public void FindPackagesGetAllCatalogPackageMetadata()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.MultipleLocale");
            Assert.AreEqual(1, searchResult.Count);

            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var allCatalogPackageMetadata = catalogPackage.GetAllCatalogPackageMetadata(packageVersionId);

            Assert.AreEqual(2, allCatalogPackageMetadata.Count);
            Assert.AreEqual("zh-CN", allCatalogPackageMetadata[0].Locale);
            Assert.AreEqual("en-GB", allCatalogPackageMetadata[1].Locale);
        }
    }
}