// -----------------------------------------------------------------------------
// <copyright file="ManifestUnitTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetUtilInterop.UnitTests.APIUnitTests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Reflection;
    using Microsoft.WinGetUtil.Api;
    using Microsoft.WinGetUtil.Common;
    using Microsoft.WinGetUtil.Manifest.V1;
    using Microsoft.WinGetUtil.Models.V1;
    using WinGetUtilInterop.UnitTests.Common;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// API manifests tests.
    /// </summary>
    public class ManifestUnitTests
    {
        private static string testCollateralDir = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TestCollateral");
        private readonly ITestOutputHelper log;

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
        [FactSkipx64CI]
        public void CreateShadowManifest()
        {
            var input = Path.Combine(testCollateralDir, "Shadow");
            var mergedManifestPath = Path.GetTempFileName();
            var logFile = Path.GetTempFileName();

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

            var enGBLocale = manifest.Localization.Where(l => l.PackageLocale == "en-gb").FirstOrDefault();
            Assert.NotNull(enGBLocale);
            Assert.Single(enGBLocale.Icons);
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

        /// <summary>
        /// Test serializing the shadow manifest.
        /// </summary>
        [Fact]
        public void SerializeShadowManifest()
        {
            var shadowManifest = ManifestShadow.CreateManifest();
            shadowManifest.Id = "Package.package";
            shadowManifest.Version = "1.0";
            shadowManifest.PackageLocale = "en-US";
            shadowManifest.ManifestVersion = "1.5";
            shadowManifest.Icons = new List<ManifestIcon>
            {
                new ManifestIcon()
                {
                    IconUrl = "iconUrl",
                    IconFileType = "fileType",
                    IconResolution = "iconResolution",
                    IconTheme = "iconTheme",
                    IconSha256 = "iconSha256",
                },
            };
            shadowManifest.Localization = new List<ManifestShadowLocalization>
            {
                new ManifestShadowLocalization()
                {
                    PackageLocale = "es-MX",
                    Icons = new List<ManifestIcon>()
                    {
                        new ManifestIcon()
                        {
                            IconUrl = "iconUrl-esMX",
                            IconFileType = "fileType-esMX",
                            IconResolution = "iconResolution-esMX",
                            IconTheme = "iconTheme-esMX",
                            IconSha256 = "iconSha256-esMX",
                        },
                    },
                },
                new ManifestShadowLocalization()
                {
                    PackageLocale = "de-DE",
                    Icons = new List<ManifestIcon>()
                    {
                        new ManifestIcon()
                        {
                            IconUrl = "iconUrl-de-DE",
                            IconFileType = "fileType-de-DE",
                            IconResolution = "iconResolution-de-DE",
                            IconTheme = "iconTheme-de-DE",
                            IconSha256 = "iconSha256-de-DE",
                        },
                    },
                },
            };

            var serialized = shadowManifest.Serialize();
            Assert.Equal(File.ReadAllText(Path.Combine(testCollateralDir, "ExpectedShadowManifest.yaml")), serialized);
            this.log.WriteLine(serialized);
        }

        /// <summary>
        /// Accessing Diagnostics without ReturnResponseAsJSON throws InvalidOperationException.
        /// </summary>
        [Fact]
        public void CreateManifestResult_DiagnosticsThrowsWithoutJsonFlag()
        {
            using var result = new CreateManifestResult(isValid: true, message: null, manifestHandle: null);
            Assert.Throws<InvalidOperationException>(() => result.Diagnostics);
        }

        /// <summary>
        /// CreateManifest with ReturnResponseAsJSON and a valid manifest returns empty Diagnostics.
        /// </summary>
        [Fact]
        public void CreateManifest_WithJsonFlag_ValidManifest_ReturnsDiagnosticsEmpty()
        {
            var input = Path.Combine(testCollateralDir, "Shadow");
            var mergedManifestPath = Path.GetTempFileName();
            var logFile = Path.GetTempFileName();

            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(logFile);
            using var result = factory.CreateManifest(
                input,
                mergedManifestPath,
                WinGetCreateManifestOption.SchemaAndSemanticValidation
                    | WinGetCreateManifestOption.AllowShadowManifest
                    | WinGetCreateManifestOption.ReturnResponseAsJson);

            Assert.True(result.IsValid);
            Assert.Null(result.Message);
            Assert.NotNull(result.Diagnostics);
            Assert.Empty(result.Diagnostics);
        }

        /// <summary>
        /// CreateManifest with ReturnResponseAsJSON and an invalid manifest returns structured Diagnostics.
        /// </summary>
        [Fact]
        public void CreateManifest_WithJsonFlag_InvalidManifest_ReturnsDiagnosticsPopulated()
        {
            // V1ManifestInfoMissingRequiredPackageLocale.yaml is a locale-type manifest without PackageLocale.
            var input = Path.Combine(testCollateralDir, "V1ManifestInfoMissingRequiredPackageLocale.yaml");
            var logFile = Path.GetTempFileName();

            var factory = new WinGetFactory();
            using var log = factory.LoggingInit(logFile);
            using var result = factory.CreateManifest(
                input,
                mergedManifestPath: null,
                WinGetCreateManifestOption.SchemaAndSemanticValidation | WinGetCreateManifestOption.ReturnResponseAsJson);

            Assert.False(result.IsValid);
            Assert.NotNull(result.Message);
            Assert.NotEmpty(result.Message);

            var diagnostics = result.Diagnostics;
            Assert.NotNull(diagnostics);
            Assert.NotEmpty(diagnostics);

            // At least one diagnostic should be an error.
            Assert.Contains(diagnostics, d => d.Level == ManifestDiagnosticLevel.Error);

            // Every diagnostic must have a non-empty message and a known (non-Unknown) error ID.
            Assert.All(diagnostics, d =>
            {
                Assert.NotEqual(ManifestErrorId.Unknown, d.ErrorId);
                Assert.False(string.IsNullOrEmpty(d.Message));
            });

            // The Message property must equal the fullMessage from the JSON (same content as without the flag).
            using var resultWithoutJson = factory.CreateManifest(
                input,
                mergedManifestPath: null,
                WinGetCreateManifestOption.SchemaAndSemanticValidation);

            Assert.Equal(resultWithoutJson.Message, result.Message);
        }
    }
}
