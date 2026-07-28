// -----------------------------------------------------------------------------
// <copyright file="FindPackagesInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Interop
{
    using System;
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
            Assert.That(searchResult.Count, Is.Zero);
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
            Assert.That(searchResult.Count, Is.EqualTo(2));
        }

        /// <summary>
        /// Test to find a package and verify the CatalogPackageMetadata COM output.
        /// </summary>
        [Test]
        public void FindPackagesVerifyDefaultLocaleFields()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.CatalogPackageMetadata");

            Assert.That(searchResult.Count, Is.EqualTo(1));

            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var packageVersionInfo = catalogPackage.GetPackageVersionInfo(packageVersionId);
            var catalogPackageMetadata = packageVersionInfo.GetCatalogPackageMetadata();

            Assert.That(catalogPackageMetadata.Author, Is.EqualTo("testAuthor"));
            Assert.That(catalogPackageMetadata.Publisher, Is.EqualTo("AppInstallerTest"));
            Assert.That(catalogPackageMetadata.PublisherUrl, Is.EqualTo("https://testPublisherUrl.com"));
            Assert.That(catalogPackageMetadata.PublisherSupportUrl, Is.EqualTo("https://testPublisherSupportUrl.com"));
            Assert.That(catalogPackageMetadata.PrivacyUrl, Is.EqualTo("https://testPrivacyUrl.com"));

            Assert.That(catalogPackageMetadata.PackageUrl, Is.EqualTo("https://testPackageUrl.com"));
            Assert.That(catalogPackageMetadata.License, Is.EqualTo("testLicense"));
            Assert.That(catalogPackageMetadata.LicenseUrl, Is.EqualTo("https://testLicenseUrl.com"));
            Assert.That(catalogPackageMetadata.Copyright, Is.EqualTo("testCopyright"));
            Assert.That(catalogPackageMetadata.CopyrightUrl, Is.EqualTo("https://testCopyrightUrl.com"));
            Assert.That(catalogPackageMetadata.Description, Is.EqualTo("testDescription"));
            Assert.That(catalogPackageMetadata.ShortDescription, Is.EqualTo("testShortDescription"));
            Assert.That(catalogPackageMetadata.PurchaseUrl, Is.EqualTo("https://testPurchaseUrl.com"));

            var tags = catalogPackageMetadata.Tags;
            Assert.That(tags.Count, Is.EqualTo(2));
            Assert.That(tags[0], Is.EqualTo("tag1"));
            Assert.That(tags[1], Is.EqualTo("tag2"));
            Assert.That(catalogPackageMetadata.ReleaseNotes, Is.EqualTo("testReleaseNotes"));
            Assert.That(catalogPackageMetadata.ReleaseNotesUrl, Is.EqualTo("https://testReleaseNotes.net"));
            Assert.That(catalogPackageMetadata.InstallationNotes, Is.EqualTo("testInstallationNotes"));

            var packageAgreements = catalogPackageMetadata.Agreements;
            Assert.That(packageAgreements.Count, Is.EqualTo(1));

            var agreement = packageAgreements[0];
            Assert.That(agreement.Label, Is.EqualTo("testAgreementLabel"));
            Assert.That(agreement.Text, Is.EqualTo("testAgreementText"));
            Assert.That(agreement.Url, Is.EqualTo("https://testAgreementUrl.net"));

            var documentations = catalogPackageMetadata.Documentations;
            Assert.That(documentations.Count, Is.EqualTo(1));

            var documentation = documentations[0];
            Assert.That(documentation.DocumentLabel, Is.EqualTo("testDocumentLabel"));
            Assert.That(documentation.DocumentUrl, Is.EqualTo("https://testDocumentUrl.com"));

            var icons = catalogPackageMetadata.Icons;
            Assert.That(icons.Count, Is.EqualTo(1));

            var icon = icons[0];
            Assert.That(icon.Url, Is.EqualTo("https://testIcon"));
            Assert.That(icon.FileType, Is.EqualTo(IconFileType.Ico));
            Assert.That(icon.Theme, Is.EqualTo(IconTheme.Default));
            Assert.That(icon.Resolution, Is.EqualTo(IconResolution.Custom));
            Assert.That(icon.Sha256, Is.EqualTo(Convert.FromHexString("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8123")));
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
            var packageVersionInfo = catalogPackage.GetPackageVersionInfo(packageVersionId);
            Assert.That((Action)(() => packageVersionInfo.GetCatalogPackageMetadata("badLocale")), Throws.TypeOf<System.ArgumentException>());
        }

        /// <summary>
        /// Verifies that the correct CatalogPackageMetadata is exposed when specifying a locale.
        /// </summary>
        [Test]
        public void FindPackagesGetCatalogPackageMetadataLocale()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.MultipleLocale");
            Assert.That(searchResult.Count, Is.EqualTo(1));

            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var packageVersionInfo = catalogPackage.GetPackageVersionInfo(packageVersionId);
            var catalogPackageMetadata = packageVersionInfo.GetCatalogPackageMetadata("zh-CN");

            Assert.That(catalogPackageMetadata.Locale, Is.EqualTo("zh-CN"));
            Assert.That(catalogPackageMetadata.License, Is.EqualTo("localeLicense"));
            Assert.That(catalogPackageMetadata.PackageName, Is.EqualTo("localePackageName"));
            Assert.That(catalogPackageMetadata.Publisher, Is.EqualTo("localePublisher"));
            Assert.That(catalogPackageMetadata.ReleaseNotes, Is.EqualTo("localeReleaseNotes"));
            Assert.That(catalogPackageMetadata.ReleaseNotesUrl, Is.EqualTo("https://localeReleaseNotesUrl.com"));
            Assert.That(catalogPackageMetadata.PurchaseUrl, Is.EqualTo("https://localePurchaseUrl.com"));

            var tags = catalogPackageMetadata.Tags;
            Assert.That(tags.Count, Is.EqualTo(2));
            Assert.That(tags[0], Is.EqualTo("tag1"));
            Assert.That(tags[1], Is.EqualTo("tag2"));

            var packageAgreements = catalogPackageMetadata.Agreements;
            Assert.That(packageAgreements.Count, Is.EqualTo(1));

            var agreement = packageAgreements[0];
            Assert.That(agreement.Label, Is.EqualTo("localeAgreementLabel"));
            Assert.That(agreement.Text, Is.EqualTo("localeAgreement"));
            Assert.That(agreement.Url, Is.EqualTo("https://localeAgreementUrl.net"));

            var documentations = catalogPackageMetadata.Documentations;
            Assert.That(documentations.Count, Is.EqualTo(1));

            var documentation = documentations[0];
            Assert.That(documentation.DocumentLabel, Is.EqualTo("localeDocumentLabel"));
            Assert.That(documentation.DocumentUrl, Is.EqualTo("https://localeDocumentUrl.com"));

            var icons = catalogPackageMetadata.Icons;
            Assert.That(icons.Count, Is.EqualTo(1));

            var icon = icons[0];
            Assert.That(icon.Url, Is.EqualTo("https://localeTestIcon"));
            Assert.That(icon.FileType, Is.EqualTo(IconFileType.Png));
            Assert.That(icon.Theme, Is.EqualTo(IconTheme.Light));
            Assert.That(icon.Resolution, Is.EqualTo(IconResolution.Square32));
            Assert.That(icon.Sha256, Is.EqualTo(Convert.FromHexString("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8321")));
        }

        /// <summary>
        /// Verifies that GetCatalogPackageMetadata returns the correct metadata based on the specified locale.
        /// </summary>
        [Test]
        public void FindPackagesGetAllCatalogPackageMetadata()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.MultipleLocale");
            Assert.That(searchResult.Count, Is.EqualTo(1));

            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var packageVersionInfo = catalogPackage.GetPackageVersionInfo(packageVersionId);

            var catalogPackageMetadata1 = packageVersionInfo.GetCatalogPackageMetadata("zh-CN");
            Assert.That(catalogPackageMetadata1.Locale, Is.EqualTo("zh-CN"));

            var catalogPackageMetadata2 = packageVersionInfo.GetCatalogPackageMetadata("en-GB");
            Assert.That(catalogPackageMetadata2.Locale, Is.EqualTo("en-GB"));
            Assert.That(catalogPackageMetadata2.PackageName, Is.EqualTo("packageNameUK"));
        }

        /// <summary>
        /// Verifies that GetCatalogPackageMetadata returns the correct metadata based on the specified locale.
        /// </summary>
        [Test]
        public void FindPackagesGetVersionMetadata()
        {
            var searchResult = this.FindAllPackages(this.testSource, PackageMatchField.Id, PackageFieldMatchOption.Equals, "AppInstallerTest.MultipleLocale");
            Assert.That(searchResult.Count, Is.EqualTo(1));

            var catalogPackage = searchResult[0].CatalogPackage;
            var packageVersionId = catalogPackage.AvailableVersions[0];
            var packageVersionInfo = catalogPackage.GetPackageVersionInfo(packageVersionId);

            string metadata = packageVersionInfo.GetMetadata(PackageVersionMetadataField.SilentUninstallCommand);
            Assert.That(metadata, Is.Empty);
        }
    }
}
