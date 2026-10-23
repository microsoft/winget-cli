// -----------------------------------------------------------------------------
// <copyright file="SourceAgreementsInterop.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
namespace AppInstallerCLIE2ETests.Interop
{
    using System;
    using System.Threading.Tasks;
    using AppInstallerCLIE2ETests.Helpers;
    using Microsoft.Management.Deployment;
    using Microsoft.Management.Deployment.Projection;
    using NUnit.Framework;

    /// <summary>
    /// Tests for the source agreement acceptance state on PackageCatalogReference.
    /// </summary>
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.InProcess), Category = nameof(InstanceInitializersSource.InProcess))]
    [TestFixtureSource(typeof(InstanceInitializersSource), nameof(InstanceInitializersSource.OutOfProcess), Category = nameof(InstanceInitializersSource.OutOfProcess))]
    public class SourceAgreementsInterop : BaseInterop
    {
        private const int ErrorIllegalStateChange = unchecked((int)0x8000000D);

        private readonly AgreementsRestSource source = new AgreementsRestSource("TestAgreementsRestSource");

        private PackageManager packageManager;

        /// <summary>
        /// Initializes a new instance of the <see cref="SourceAgreementsInterop"/> class.
        /// </summary>
        /// <param name="initializer">initializer.</param>
        public SourceAgreementsInterop(IInstanceInitializer initializer)
            : base(initializer)
        {
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            this.source.Remove();
            this.packageManager = this.TestFactory.CreatePackageManager();
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            this.source.Remove();
        }

        /// <summary>
        /// A source without agreements or implicit agreement fields is satisfied.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task NoAgreements_Satisfied()
        {
            this.source.WriteInformation(agreementsIdentifier: null, requiresMarket: false);
            var catalogRef = await this.AddCatalogAsync();

            Assert.That(catalogRef.SourceAgreements.Count, Is.EqualTo(0));
            Assert.That(catalogRef.ImplicitAgreementFields, Is.EqualTo(ImplicitAgreementFields.None));
            Assert.That(catalogRef.SourceAgreementsSatisfied, Is.True);
        }

        /// <summary>
        /// Adding a source accepts its agreements; changing them requires acceptance again, which is persisted by SaveAcceptedSourceAgreements.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task ChangedAgreements_NotSatisfiedUntilSaved()
        {
            this.source.WriteInformation("AgreementsV1", requiresMarket: true);
            var catalogRef = await this.AddCatalogAsync();

            Assert.That(catalogRef.SourceAgreements.Count, Is.EqualTo(1));
            Assert.That(catalogRef.ImplicitAgreementFields, Is.EqualTo(ImplicitAgreementFields.Market));
            Assert.That(catalogRef.SourceAgreementsSatisfied, Is.True, "Adding a source records acceptance of its agreements.");

            // A new reference is needed, as the source information is retrieved once per reference.
            this.source.WriteInformation("AgreementsV2", requiresMarket: true);
            catalogRef = this.packageManager.GetPackageCatalogByName(this.source.Name);
            Assert.That(catalogRef.SourceAgreementsSatisfied, Is.False);

            catalogRef.SaveAcceptedSourceAgreements();
            Assert.That(catalogRef.SourceAgreementsSatisfied, Is.True);

            // Verify that the acceptance was persisted.
            catalogRef = this.packageManager.GetPackageCatalogByName(this.source.Name);
            Assert.That(catalogRef.SourceAgreementsSatisfied, Is.True);
        }

        /// <summary>
        /// The agreement members are not available on a composite catalog reference.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Test]
        public async Task Composite_Throws()
        {
            this.source.WriteInformation("AgreementsV1", requiresMarket: true);
            var catalogRef = await this.AddCatalogAsync();

            var options = this.TestFactory.CreateCreateCompositePackageCatalogOptions();
            options.Catalogs.Add(catalogRef);
            options.CompositeSearchBehavior = CompositeSearchBehavior.AllCatalogs;
            var compositeRef = this.packageManager.CreateCompositePackageCatalog(options);
            Assert.That(compositeRef.IsComposite, Is.True);

            Assert.That(() => compositeRef.SourceAgreementsSatisfied, Throws.InstanceOf<Exception>().With.Property("HResult").EqualTo(ErrorIllegalStateChange));
            Assert.That(() => compositeRef.ImplicitAgreementFields, Throws.InstanceOf<Exception>().With.Property("HResult").EqualTo(ErrorIllegalStateChange));
            Assert.That(() => compositeRef.SaveAcceptedSourceAgreements(), Throws.InstanceOf<Exception>().With.Property("HResult").EqualTo(ErrorIllegalStateChange));
        }

        private async Task<PackageCatalogReference> AddCatalogAsync()
        {
            var options = this.TestFactory.CreateAddPackageCatalogOptions();
            options.Name = this.source.Name;
            options.SourceUri = this.source.Url;
            options.Type = Constants.RestTestSourceType;
            options.TrustLevel = PackageCatalogTrustLevel.Trusted;

            var addResult = await this.packageManager.AddPackageCatalogAsync(options);
            Assert.That(addResult, Is.Not.Null);
            Assert.That(addResult.Status, Is.EqualTo(AddPackageCatalogStatus.Ok), $"Failed to add REST test source. Error: {addResult.ExtendedErrorCode?.HResult:X}");

            var catalogRef = this.packageManager.GetPackageCatalogByName(this.source.Name);
            Assert.That(catalogRef, Is.Not.Null);
            return catalogRef;
        }
    }
}
