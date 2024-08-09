// -----------------------------------------------------------------------------
// <copyright file="V1ManifestReadTest.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetUtilInterop.UnitTests.ManifestUnitTest
{
    using System.IO;
    using System.Reflection;
    using Microsoft.WinGetUtil.Models.V1;
    using Microsoft.WinGetUtil.UnitTests.Common.Logging;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Manifest equality tests.
    /// </summary>
    public class V1ManifestReadTest
    {
        private ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="V1ManifestReadTest"/> class.
        /// </summary>
        /// <param name="log">Output Helper.</param>
        public V1ManifestReadTest(ITestOutputHelper log)
        {
            this.log = log;
        }

        private enum TestManifestVersion
        {
            V100,
            V110,
            V160,
            V170,
        }

        /// <summary>
        /// Read v1 manifest.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void ReadV1ManifestsAndVerifyContents()
        {
            Manifest v100manifest = Manifest.CreateManifestFromPath(
                Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TestCollateral", ManifestStrings.V100ManifestMerged));

            this.ValidateManifestFields(v100manifest, TestManifestVersion.V100);

            Manifest v110manifest = Manifest.CreateManifestFromPath(
                Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TestCollateral", ManifestStrings.V110ManifestMerged));

            this.ValidateManifestFields(v110manifest, TestManifestVersion.V110);

            Manifest v160manifest = Manifest.CreateManifestFromPath(
                Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TestCollateral", ManifestStrings.V160ManifestMerged));

            this.ValidateManifestFields(v160manifest, TestManifestVersion.V160);

            Manifest v170manifest = Manifest.CreateManifestFromPath(
                Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TestCollateral", ManifestStrings.V170ManifestMerged));

            this.ValidateManifestFields(v170manifest, TestManifestVersion.V170);
        }

        /// <summary>
        /// Read v1 manifest without additional localization.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void ReadV1ManifestNoLocalization()
        {
            Manifest v1manifest = Manifest.CreateManifestFromPath(
                Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "TestCollateral", ManifestStrings.V1ManifestNoLocalization));

            // Calling GetURIs() on manifest without localization should not fail.
            v1manifest.GetURIs();
        }

        /// <summary>
        /// Read bad min manifest info.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void ReadV1LocaleManifestInfoNoPackageLocale()
        {
            Assert.Throws<InvalidDataException>(
                () =>
                {
                    _ = MinManifestInfo.CreateManifestInfoFromPath(
                            Path.Combine(
                                Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),
                                "TestCollateral",
                                ManifestStrings.V1ManifestInfoMissingRequiredPackageLocale));
                });
        }

        private void ValidateManifestFields(Manifest manifest, TestManifestVersion manifestVersion)
        {
            Assert.Equal("microsoft.msixsdk", manifest.Id);
            Assert.Equal("1.7.32", manifest.Version);

            // Default locale
            Assert.Equal("en-US", manifest.PackageLocale);
            Assert.Equal("Microsoft", manifest.Publisher);
            Assert.Equal("https://www.microsoft.com", manifest.PublisherUrl);
            Assert.Equal("https://www.microsoft.com/support", manifest.PublisherSupportUrl);
            Assert.Equal("https://www.microsoft.com/privacy", manifest.PrivacyUrl);
            Assert.Equal("Microsoft", manifest.Author);
            Assert.Equal("MSIX SDK", manifest.PackageName);
            Assert.Equal("https://www.microsoft.com/msixsdk/home", manifest.PackageUrl);
            Assert.Equal("MIT License", manifest.License);
            Assert.Equal("https://www.microsoft.com/msixsdk/license", manifest.LicenseUrl);
            Assert.Equal("Copyright Microsoft Corporation", manifest.Copyright);
            Assert.Equal("https://www.microsoft.com/msixsdk/copyright", manifest.CopyrightUrl);
            Assert.Equal("This is MSIX SDK", manifest.ShortDescription);
            Assert.Equal("The MSIX SDK project is an effort to enable developers", manifest.Description);
            Assert.Equal("msixsdk", manifest.Moniker);
            Assert.Equal(2, manifest.Tags.Count);
            Assert.Equal("appxsdk", manifest.Tags[0]);
            Assert.Equal("msixsdk", manifest.Tags[1]);

            if (manifestVersion >= TestManifestVersion.V110)
            {
                Assert.Equal("Default release notes", manifest.ReleaseNotes);
                Assert.Equal("https://DefaultReleaseNotes.net", manifest.ReleaseNotesUrl);
                Assert.Single(manifest.Agreements);
                Assert.Equal("DefaultLabel", manifest.Agreements[0].AgreementLabel);
                Assert.Equal("DefaultText", manifest.Agreements[0].Agreement);
                Assert.Equal("https://DefaultAgreementUrl.net", manifest.Agreements[0].AgreementUrl);
            }

            if (manifestVersion >= TestManifestVersion.V160)
            {
                Assert.Equal("Default installation notes", manifest.InstallationNotes);
                Assert.Equal("https://DefaultPurchaseUrl.com", manifest.PurchaseUrl);

                Assert.Single(manifest.Documentations);
                ManifestDocumentation manifestDocumentation = manifest.Documentations[0];

                Assert.Equal("Default document label", manifestDocumentation.DocumentLabel);
                Assert.Equal("https://DefaultDocumentUrl.com", manifestDocumentation.DocumentUrl);

                Assert.Single(manifest.Icons);
                ManifestIcon icon = manifest.Icons[0];

                Assert.Equal("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8123", icon.IconSha256);
                Assert.Equal("default", icon.IconTheme);
                Assert.Equal("https://testIcon", icon.IconUrl);
                Assert.Equal("custom", icon.IconResolution);
                Assert.Equal("ico", icon.IconFileType);
            }

            // Default installer
            Assert.Equal("en-US", manifest.InstallerLocale);
            Assert.Equal(2, manifest.Platform.Count);
            Assert.Equal("Windows.Desktop", manifest.Platform[0]);
            Assert.Equal("Windows.Universal", manifest.Platform[1]);
            Assert.Equal("10.0.0.0", manifest.MinimumOSVersion);

            Assert.Equal("zip", manifest.InstallerType);
            Assert.Equal("machine", manifest.Scope);
            Assert.Equal(3, manifest.InstallModes.Count);
            Assert.Equal("interactive", manifest.InstallModes[0]);
            Assert.Equal("silent", manifest.InstallModes[1]);
            Assert.Equal("silentWithProgress", manifest.InstallModes[2]);

            var defaultSwitches = manifest.Switches;
            Assert.Equal("/custom", defaultSwitches.Custom);
            Assert.Equal("/silentwithprogress", defaultSwitches.SilentWithProgress);
            Assert.Equal("/silence", defaultSwitches.Silent);
            Assert.Equal("/interactive", defaultSwitches.Interactive);
            Assert.Equal("/log=<LOGPATH>", defaultSwitches.Log);
            Assert.Equal("/dir=<INSTALLPATH>", defaultSwitches.InstallLocation);
            Assert.Equal("/upgrade", defaultSwitches.Upgrade);

            Assert.Equal(2, manifest.InstallerSuccessCodes.Count);
            Assert.Equal(1, manifest.InstallerSuccessCodes[0]);
            Assert.Equal(0x80070005, manifest.InstallerSuccessCodes[1]);
            Assert.Equal("uninstallPrevious", manifest.UpgradeBehavior);
            Assert.Equal(2, manifest.Commands.Count);
            Assert.Equal("makemsix", manifest.Commands[0]);
            Assert.Equal("makeappx", manifest.Commands[1]);
            Assert.Equal(2, manifest.Protocols.Count);
            Assert.Equal("protocol1", manifest.Protocols[0]);
            Assert.Equal("protocol2", manifest.Protocols[1]);
            Assert.Equal(4, manifest.FileExtensions.Count);
            Assert.Equal("appx", manifest.FileExtensions[0]);
            Assert.Equal("msix", manifest.FileExtensions[1]);
            Assert.Equal("appxbundle", manifest.FileExtensions[2]);
            Assert.Equal("msixbundle", manifest.FileExtensions[3]);

            Assert.Single(manifest.Dependencies.WindowsFeatures);
            Assert.Equal("IIS", manifest.Dependencies.WindowsFeatures[0]);
            Assert.Single(manifest.Dependencies.WindowsLibraries);
            Assert.Equal("VC Runtime", manifest.Dependencies.WindowsLibraries[0]);
            Assert.Single(manifest.Dependencies.PackageDependencies);
            Assert.Equal("Microsoft.MsixSdkDep", manifest.Dependencies.PackageDependencies[0].PackageIdentifier);
            Assert.Equal("1.0.0", manifest.Dependencies.PackageDependencies[0].MinimumVersion);
            Assert.Single(manifest.Dependencies.ExternalDependencies);
            Assert.Equal("Outside dependencies", manifest.Dependencies.ExternalDependencies[0]);

            Assert.Single(manifest.Capabilities);
            Assert.Equal("internetClient", manifest.Capabilities[0]);
            Assert.Single(manifest.RestrictedCapabilities);
            Assert.Equal("runFullTrust", manifest.RestrictedCapabilities[0]);
            Assert.Equal("Microsoft.DesktopAppInstaller_8wekyb3d8bbwe", manifest.PackageFamilyName);
            Assert.Equal("{Foo}", manifest.ProductCode);

            if (manifestVersion >= TestManifestVersion.V110)
            {
                Assert.Equal("2021-01-01", manifest.ReleaseDate);
                Assert.True(manifest.InstallerAbortsTerminal);
                Assert.True(manifest.InstallLocationRequired);
                Assert.True(manifest.RequireExplicitUpgrade);
                Assert.Equal("elevatesSelf", manifest.ElevationRequirement);
                Assert.Single(manifest.UnsupportedOSArchitectures);
                Assert.Equal("arm", manifest.UnsupportedOSArchitectures[0]);
                Assert.Single(manifest.AppsAndFeaturesEntries);
                Assert.Equal("DisplayName", manifest.AppsAndFeaturesEntries[0].DisplayName);
                Assert.Equal("DisplayVersion", manifest.AppsAndFeaturesEntries[0].DisplayVersion);
                Assert.Equal("Publisher", manifest.AppsAndFeaturesEntries[0].Publisher);
                Assert.Equal("ProductCode", manifest.AppsAndFeaturesEntries[0].ProductCode);
                Assert.Equal("UpgradeCode", manifest.AppsAndFeaturesEntries[0].UpgradeCode);
                Assert.Equal("exe", manifest.AppsAndFeaturesEntries[0].InstallerType);
                Assert.Single(manifest.Markets.AllowedMarkets);
                Assert.Equal("US", manifest.Markets.AllowedMarkets[0]);
                Assert.Equal(2, manifest.ExpectedReturnCodes.Count);
                Assert.Equal(2, manifest.ExpectedReturnCodes[0].InstallerReturnCode);
                Assert.Equal("contactSupport", manifest.ExpectedReturnCodes[0].ReturnResponse);
            }

            if (manifestVersion >= TestManifestVersion.V160)
            {
                Assert.Equal("msi", manifest.NestedInstallerType);
                Assert.Single(manifest.NestedInstallerFiles);
                InstallerNestedInstallerFile installerNestedInstallerFile = manifest.NestedInstallerFiles[0];
                Assert.Equal("RelativeFilePath", installerNestedInstallerFile.RelativeFilePath);
                Assert.Equal("PortableCommandAlias", installerNestedInstallerFile.PortableCommandAlias);

                InstallerInstallationMetadata installerInstallationMetadata = manifest.InstallationMetadata;
                Assert.Equal("%ProgramFiles%\\TestApp", installerInstallationMetadata.DefaultInstallLocation);
                Assert.Single(installerInstallationMetadata.Files);

                ManifestInstallerFile installerFile = installerInstallationMetadata.Files[0];
                Assert.Equal("main.exe", installerFile.RelativeFilePath);
                Assert.Equal("DisplayName", installerFile.DisplayName);
                Assert.Equal("/arg", installerFile.InvocationParameter);
                Assert.Equal("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82", installerFile.FileSha256);

                Assert.Single(manifest.UnsupportedArguments);
                Assert.Equal("log", manifest.UnsupportedArguments[0]);

                Assert.Single(manifest.UnsupportedOSArchitectures);
                Assert.Equal("arm", manifest.UnsupportedOSArchitectures[0]);

                Assert.True(manifest.DisplayInstallWarnings);
                Assert.True(manifest.DownloadCommandProhibited);
            }

            if (manifestVersion >= TestManifestVersion.V170)
            {
                Assert.Equal("/repair", defaultSwitches.Repair);
                Assert.Equal("uninstaller", manifest.RepairBehavior);
            }

            // Individual installers
            Assert.Equal(2, manifest.Installers.Count);
            ManifestInstaller installer1 = manifest.Installers[0];
            Assert.Equal("x86", installer1.Arch);
            Assert.Equal("en-GB", installer1.InstallerLocale);
            Assert.Single(installer1.Platform);
            Assert.Equal("Windows.Desktop", installer1.Platform[0]);
            Assert.Equal("10.0.1.0", installer1.MinimumOSVersion);
            Assert.Equal("msix", installer1.InstallerType);
            Assert.Equal("https://www.microsoft.com/msixsdk/msixsdkx86.msix", installer1.Url);
            Assert.Equal("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82", installer1.Sha256);
            Assert.Equal("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82", installer1.SignatureSha256);
            Assert.Equal("user", installer1.Scope);
            Assert.Single(installer1.InstallModes);
            Assert.Equal("interactive", installer1.InstallModes[0]);

            var installer1Switches = installer1.Switches;
            Assert.Equal("/c", installer1Switches.Custom);
            Assert.Equal("/sp", installer1Switches.SilentWithProgress);
            Assert.Equal("/s", installer1Switches.Silent);
            Assert.Equal("/i", installer1Switches.Interactive);
            Assert.Equal("/l=<LOGPATH>", installer1Switches.Log);
            Assert.Equal("/d=<INSTALLPATH>", installer1Switches.InstallLocation);
            Assert.Equal("/u", installer1Switches.Upgrade);

            Assert.Equal("install", installer1.UpgradeBehavior);
            Assert.Equal(2, installer1.Commands.Count);
            Assert.Equal("makemsixPreview", installer1.Commands[0]);
            Assert.Equal("makeappxPreview", installer1.Commands[1]);
            Assert.Equal(2, installer1.Protocols.Count);
            Assert.Equal("protocol1preview", installer1.Protocols[0]);
            Assert.Equal("protocol2preview", installer1.Protocols[1]);
            Assert.Equal(4, installer1.FileExtensions.Count);
            Assert.Equal("appxbundle", installer1.FileExtensions[0]);
            Assert.Equal("msixbundle", installer1.FileExtensions[1]);
            Assert.Equal("appx", installer1.FileExtensions[2]);
            Assert.Equal("msix", installer1.FileExtensions[3]);

            Assert.Single(installer1.Dependencies.WindowsFeatures);
            Assert.Equal("PreviewIIS", installer1.Dependencies.WindowsFeatures[0]);
            Assert.Single(installer1.Dependencies.WindowsLibraries);
            Assert.Equal("Preview VC Runtime", installer1.Dependencies.WindowsLibraries[0]);
            Assert.Single(installer1.Dependencies.PackageDependencies);
            Assert.Equal("Microsoft.MsixSdkDepPreview", installer1.Dependencies.PackageDependencies[0].PackageIdentifier);
            Assert.Single(installer1.Dependencies.ExternalDependencies);
            Assert.Equal("Preview Outside dependencies", installer1.Dependencies.ExternalDependencies[0]);

            Assert.Single(installer1.Capabilities);
            Assert.Equal("internetClientPreview", installer1.Capabilities[0]);
            Assert.Single(installer1.RestrictedCapabilities);
            Assert.Equal("runFullTrustPreview", installer1.RestrictedCapabilities[0]);
            Assert.Equal("Microsoft.DesktopAppInstallerPreview_8wekyb3d8bbwe", installer1.PackageFamilyName);

            if (manifestVersion >= TestManifestVersion.V110)
            {
                Assert.Equal("2021-02-02", installer1.ReleaseDate);
                Assert.True(!installer1.InstallerAbortsTerminal);
                Assert.True(!installer1.InstallLocationRequired);
                Assert.True(!installer1.RequireExplicitUpgrade);
                Assert.Equal("elevationRequired", installer1.ElevationRequirement);
                Assert.Single(installer1.UnsupportedOSArchitectures);
                Assert.Equal("arm64", installer1.UnsupportedOSArchitectures[0]);
                Assert.Single(installer1.Markets.ExcludedMarkets);
                Assert.Equal("US", installer1.Markets.ExcludedMarkets[0]);
                Assert.Single(installer1.ExpectedReturnCodes);
                Assert.Equal(2, installer1.ExpectedReturnCodes[0].InstallerReturnCode);
                Assert.Equal("contactSupport", installer1.ExpectedReturnCodes[0].ReturnResponse);
            }

            ManifestInstaller installer2 = manifest.Installers[1];
            Assert.Equal("x64", installer2.Arch);
            Assert.Equal("exe", installer2.InstallerType);
            Assert.Equal("https://www.microsoft.com/msixsdk/msixsdkx64.exe", installer2.Url);
            Assert.Equal("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82", installer2.Sha256);
            Assert.Equal("{Bar}", installer2.ProductCode);

            if (manifestVersion >= TestManifestVersion.V160)
            {
                Assert.Single(installer1.InstallationMetadata.Files);
                ManifestInstallerFile installerFile2 = installer1.InstallationMetadata.Files[0];
                Assert.Equal("main2.exe", installerFile2.RelativeFilePath);
                Assert.Equal("DisplayName2", installerFile2.DisplayName);
                Assert.Equal("/arg2", installerFile2.InvocationParameter);
                Assert.Equal("79D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8C82", installerFile2.FileSha256);

                Assert.Equal("msi", installer1.NestedInstallerType);

                InstallerNestedInstallerFile installerNestedInstallerFile2 = installer1.NestedInstallerFiles[0];
                Assert.Equal("RelativeFilePath2", installerNestedInstallerFile2.RelativeFilePath);
                Assert.Equal("PortableCommandAlias2", installerNestedInstallerFile2.PortableCommandAlias);

                Assert.Single(installer1.UnsupportedArguments);
                Assert.Equal("location", installer1.UnsupportedArguments[0]);

                Assert.True(installer1.DisplayInstallWarnings);
                Assert.True(installer1.DownloadCommandProhibited);
            }

            if (manifestVersion >= TestManifestVersion.V170)
            {
                Assert.Equal("/r", installer1Switches.Repair);
                Assert.Equal("modify", installer1.RepairBehavior);
            }

            // Additional Localizations
            Assert.Single(manifest.Localization);
            ManifestLocalization localization1 = manifest.Localization[0];
            Assert.Equal("en-GB", localization1.PackageLocale);
            Assert.Equal("Microsoft UK", localization1.Publisher);
            Assert.Equal("https://www.microsoft.com/UK", localization1.PublisherUrl);
            Assert.Equal("https://www.microsoft.com/support/UK", localization1.PublisherSupportUrl);
            Assert.Equal("https://www.microsoft.com/privacy/UK", localization1.PrivacyUrl);
            Assert.Equal("Microsoft UK", localization1.Author);
            Assert.Equal("MSIX SDK UK", localization1.PackageName);
            Assert.Equal("https://www.microsoft.com/msixsdk/home/UK", localization1.PackageUrl);
            Assert.Equal("MIT License UK", localization1.License);
            Assert.Equal("https://www.microsoft.com/msixsdk/license/UK", localization1.LicenseUrl);
            Assert.Equal("Copyright Microsoft Corporation UK", localization1.Copyright);
            Assert.Equal("https://www.microsoft.com/msixsdk/copyright/UK", localization1.CopyrightUrl);
            Assert.Equal("This is MSIX SDK UK", localization1.ShortDescription);
            Assert.Equal("The MSIX SDK project is an effort to enable developers UK", localization1.Description);
            Assert.Equal(2, localization1.Tags.Count);
            Assert.Equal("appxsdkUK", localization1.Tags[0]);
            Assert.Equal("msixsdkUK", localization1.Tags[1]);

            if (manifestVersion >= TestManifestVersion.V110)
            {
                Assert.Equal("Release notes", localization1.ReleaseNotes);
                Assert.Equal("https://ReleaseNotes.net", localization1.ReleaseNotesUrl);
                Assert.Single(localization1.Agreements);
                Assert.Equal("Label", localization1.Agreements[0].AgreementLabel);
                Assert.Equal("Text", localization1.Agreements[0].Agreement);
                Assert.Equal("https://AgreementUrl.net", localization1.Agreements[0].AgreementUrl);
            }

            if (manifestVersion >= TestManifestVersion.V160)
            {
                Assert.Equal("Installation notes", localization1.InstallationNotes);
                Assert.Equal("https://PurchaseUrl.com", localization1.PurchaseUrl);

                Assert.Single(localization1.Documentations);
                ManifestDocumentation manifestDocumentation = localization1.Documentations[0];

                Assert.Equal("Document label", manifestDocumentation.DocumentLabel);
                Assert.Equal("https://DocumentUrl.com", manifestDocumentation.DocumentUrl);

                Assert.Single(localization1.Icons);
                ManifestIcon icon = localization1.Icons[0];

                Assert.Equal("69D84CA8899800A5575CE31798293CD4FEBAB1D734A07C2E51E56A28E0DF8321", icon.IconSha256);
                Assert.Equal("dark", icon.IconTheme);
                Assert.Equal("https://testIcon2", icon.IconUrl);
                Assert.Equal("32x32", icon.IconResolution);
                Assert.Equal("png", icon.IconFileType);
            }
        }

        /// <summary>
        /// Helper class with manifest strings.
        /// </summary>
        internal class ManifestStrings
        {
            /// <summary>
            /// Merged v1 manifest.
            /// </summary>
            public const string V100ManifestMerged = "V1ManifestMerged.yaml";

            /// <summary>
            /// Merged v1.1 manifest.
            /// </summary>
            public const string V110ManifestMerged = "V1_1ManifestMerged.yaml";

            /// <summary>
            /// Merged v1.6 manifest.
            /// </summary>
            public const string V160ManifestMerged = "V1_6ManifestMerged.yaml";

            /// <summary>
            /// Merged v1.7 manifest.
            /// </summary>
            public const string V170ManifestMerged = "V1_7ManifestMerged.yaml";

            /// <summary>
            /// Merged v1 manifest without localization.
            /// </summary>
            public const string V1ManifestNoLocalization = "V1ManifestNoLocalization.yaml";

            /// <summary>
            /// Merged v1 manifest missing required PackageLocale.
            /// </summary>
            public const string V1ManifestInfoMissingRequiredPackageLocale = "V1ManifestInfoMissingRequiredPackageLocale.yaml";
        }
    }
}
