// -----------------------------------------------------------------------------
// <copyright file="ManifestUnitTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetUtilInterop.UnitTests.APIUnitTests
{
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using Microsoft.WinGetUtil.Api;
    using Microsoft.WinGetUtil.Common;
    using Microsoft.WinGetUtil.Models.V1;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// API manifests tests.
    /// </summary>
    public class ManifestUnitTests
    {
        private ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ManifestUnitTests"/> class.
        /// </summary>
        /// <param name="log">Output Helper.</param>
        public ManifestUnitTests(ITestOutputHelper log)
        {
            this.log = log;
        }

        /// <summary>
        /// Test creating a manifest with shadow.
        /// </summary>
        [Fact]
        public void CreateShadowManifest()
        {
            var input = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TestCollateral", "Shadow");
            var mergedManifestPath = Path.GetTempFileName();
            var logFile = Path.GetTempFileName();
            this.log.WriteLine(mergedManifestPath);

            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(logFile);
            using var result = factory.CreateManifest(
                input,
                mergedManifestPath,
                WinGetCreateManifestOption.SchemaAndSemanticValidation | WinGetCreateManifestOption.AllowShadowManifest);

            Assert.NotNull(result);
            Assert.True(result.IsValid);
            Assert.NotNull(result.ManifestHandle);

            var manifest = Manifest.CreateManifestFromPath(mergedManifestPath);

            Assert.Equal("microsoft.msixsdk", manifest.Id);
            Assert.Equal("1.7.32", manifest.Version);
            Assert.Single(manifest.Installers);
            Assert.Equal("en-US", manifest.PackageLocale);
            Assert.Equal("Microsoft", manifest.Publisher);
            Assert.Equal("MSIX SDK", manifest.PackageName);
            Assert.Equal("MIT License", manifest.License);
            Assert.Equal("The MSIX SDK project is an effort to enable developers", manifest.Description);
            Assert.Equal("This is MSIX SDK", manifest.ShortDescription);

            Assert.Single(manifest.Icons);
            Assert.Equal("https://shadowIcon-default", manifest.Icons[0].IconUrl);
            Assert.Equal("ico", manifest.Icons[0].IconFileType);
            Assert.Equal("custom", manifest.Icons[0].IconResolution);
            Assert.Equal("default", manifest.Icons[0].IconTheme);
            Assert.Equal("1111111111111111111111111111111111111111111111111111111111111111", manifest.Icons[0].IconSha256);

            Assert.Equal(2, manifest.Localization.Count);

            var enGBLocale = manifest.Localization.Where(l => l.PackageLocale == "en-GB").FirstOrDefault();
            Assert.NotNull(enGBLocale);
            Assert.Single(enGBLocale.Icons);
            Assert.Equal("The MSIX SDK project is an effort to enable developers UK", enGBLocale.Description);
            Assert.Equal("https://shadowIcon-en-GB", enGBLocale.Icons[0].IconUrl);
            Assert.Equal("png", enGBLocale.Icons[0].IconFileType);
            Assert.Equal("32x32", enGBLocale.Icons[0].IconResolution);
            Assert.Equal("light", enGBLocale.Icons[0].IconTheme);
            Assert.Equal("2222222222222222222222222222222222222222222222222222222222222222", enGBLocale.Icons[0].IconSha256);

            var frFRLocale = manifest.Localization.Where(l => l.PackageLocale == "fr-FR").FirstOrDefault();
            Assert.NotNull(frFRLocale);
            Assert.Single(frFRLocale.Icons);
            Assert.Equal("https://shadowIcon-fr-FR", frFRLocale.Icons[0].IconUrl);
            Assert.Equal("jpeg", frFRLocale.Icons[0].IconFileType);
            Assert.Equal("20x20", frFRLocale.Icons[0].IconResolution);
            Assert.Equal("dark", frFRLocale.Icons[0].IconTheme);
            Assert.Equal("3333333333333333333333333333333333333333333333333333333333333333", frFRLocale.Icons[0].IconSha256);
        }
    }
}
