// -----------------------------------------------------------------------------
// <copyright file="DSCv3PackageResourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.Collections.Generic;
    using System.Text.Json.Serialization;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `Configure` command tests.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Closing square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
    public class DSCv3PackageResourceCommand : DSCv3ResourceTestBase
    {
        private const string DefaultPackageIdentifier = Constants.ExeInstallerPackageId;
        private const string DefaultPackageLowVersion = "1.0.0.0";
        private const string DefaultPackageMidVersion = "1.1.0.0";
        private const string DefaultPackageHighVersion = "2.0.0.0";
        private const string PackageResource = "package";
        private const string VersionPropertyName = "version";
        private const string UseLatestPropertyName = "useLatest";

        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            TestCommon.SetupTestSource();
            WinGetSettingsHelper.ConfigureLoggingLevel("verbose");
            EnsureTestResourcePresence();
        }

        /// <summary>
        /// Teardown done once after all the tests here.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTeardown()
        {
            RemoveTestPackage();
            WinGetSettingsHelper.ConfigureLoggingLevel(null);
            TestCommon.TearDownTestSource();
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            // Try clean up TestExeInstaller for failure cases where cleanup is not successful
            RemoveTestPackage();
        }

        /// <summary>
        /// Calls `get` on the `package` resource with the value not present.
        /// </summary>
        [Test]
        public void Package_Get_NotPresent()
        {
            PackageResourceData packageResourceData = new PackageResourceData() { Identifier = DefaultPackageIdentifier };

            var result = RunDSCv3Command(PackageResource, GetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            PackageResourceData output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(packageResourceData.Identifier, output.Identifier);
        }

        /// <summary>
        /// Calls `get` on the `package` resource with the package not existing.
        /// </summary>
        [Test]
        public void Package_Get_UnknownIdentifier()
        {
            PackageResourceData packageResourceData = new PackageResourceData() { Identifier = "Not.An.Existing.Identifier.123456789.ABCDEFG" };

            var result = RunDSCv3Command(PackageResource, GetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            PackageResourceData output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(packageResourceData.Identifier, output.Identifier);
        }

        /// <summary>
        /// Calls `get` on the `package` resource with the value present.
        /// </summary>
        [Test]
        public void Package_Get_Present()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageLowVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData() { Identifier = DefaultPackageIdentifier };

            var result = RunDSCv3Command(PackageResource, GetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            PackageResourceData output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageLowVersion);
        }

        /// <summary>
        /// Calls `get` on the `package` resource with the value present and supplying most inputs.
        /// </summary>
        [Test]
        public void Package_Get_MuchInput()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageLowVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                Source = Constants.TestSourceName,
                MatchOption = "equals",
            };

            var result = RunDSCv3Command(PackageResource, GetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            PackageResourceData output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageLowVersion);
        }

        /// <summary>
        /// Calls `test` on the `package` resource with the value not present.
        /// </summary>
        [Test]
        public void Package_Test_NotPresent()
        {
            PackageResourceData packageResourceData = new PackageResourceData() { Identifier = DefaultPackageIdentifier };

            var result = RunDSCv3Command(PackageResource, TestFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(packageResourceData.Identifier, output.Identifier);
            Assert.False(output.InDesiredState);

            AssertDiffState(diff, [ ExistPropertyName ]);
        }

        /// <summary>
        /// Calls `test` on the `package` resource with the value present.
        /// </summary>
        [Test]
        public void Package_Test_SimplePresent()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageLowVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData() { Identifier = DefaultPackageIdentifier };

            var result = RunDSCv3Command(PackageResource, TestFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageLowVersion);
            Assert.True(output.InDesiredState);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `test` on the `package` resource with a version that matches.
        /// </summary>
        [Test]
        public void Package_Test_VersionMatch()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageLowVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                Version = DefaultPackageLowVersion,
            };

            var result = RunDSCv3Command(PackageResource, TestFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageLowVersion);
            Assert.True(output.InDesiredState);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `test` on the `package` resource with a version that does not match.
        /// </summary>
        [Test]
        public void Package_Test_VersionMismatch()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageLowVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                Version = DefaultPackageMidVersion,
            };

            var result = RunDSCv3Command(PackageResource, TestFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageLowVersion);
            Assert.False(output.InDesiredState);

            AssertDiffState(diff, [ VersionPropertyName ]);
        }

        /// <summary>
        /// Calls `test` on the `package` resource with a version that is the latest.
        /// </summary>
        [Test]
        public void Package_Test_Latest()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                UseLatest = true,
            };

            var result = RunDSCv3Command(PackageResource, TestFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageHighVersion);
            Assert.True(output.InDesiredState);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `test` on the `package` resource with a version that is not the latest.
        /// </summary>
        [Test]
        public void Package_Test_NotLatest()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageMidVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                UseLatest = true,
            };

            var result = RunDSCv3Command(PackageResource, TestFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageMidVersion);
            Assert.False(output.InDesiredState);

            AssertDiffState(diff, [ UseLatestPropertyName ]);
        }

        /// <summary>
        /// Calls `set` on the `package` resource when the package is not present, and again afterward.
        /// </summary>
        [Test]
        public void Package_Set_SimpleRepeated()
        {
            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
            };

            var result = RunDSCv3Command(PackageResource, SetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageHighVersion, ignoreLatest: true);

            AssertDiffState(diff, [ ExistPropertyName ]);

            // Set again should be a no-op
            result = RunDSCv3Command(PackageResource, SetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (output, diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageHighVersion);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `set` on the `package` resource to ensure that it is not present.
        /// </summary>
        [Test]
        public void Package_Set_Remove()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                Exist = false,
            };

            var result = RunDSCv3Command(PackageResource, SetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(packageResourceData.Identifier, output.Identifier);

            AssertDiffState(diff, [ ExistPropertyName ]);

            // Call `get` to ensure the result
            PackageResourceData packageResourceDataForGet = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
            };

            result = RunDSCv3Command(PackageResource, GetFunction, packageResourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(packageResourceDataForGet.Identifier, output.Identifier);
        }

        /// <summary>
        /// Calls `set` on the `package` resource to request the latest version when a lower version is installed.
        /// </summary>
        [Test]
        public void Package_Set_Latest()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageMidVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                UseLatest = true,
            };

            var result = RunDSCv3Command(PackageResource, SetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageHighVersion, ignoreLatest: true);

            AssertDiffState(diff, [ UseLatestPropertyName ]);

            // Call `get` to ensure the result
            PackageResourceData packageResourceDataForGet = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
            };

            result = RunDSCv3Command(PackageResource, GetFunction, packageResourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageHighVersion);
        }

        /// <summary>
        /// Calls `set` on the `package` resource to request a specific version when a lower version is installed.
        /// </summary>
        [Test]
        public void Package_Set_SpecificVersionUpgrade()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageLowVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                Version = DefaultPackageMidVersion,
            };

            var result = RunDSCv3Command(PackageResource, SetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageMidVersion, ignoreLatest: true);

            AssertDiffState(diff, [ VersionPropertyName ]);

            // Call `get` to ensure the result
            PackageResourceData packageResourceDataForGet = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
            };

            result = RunDSCv3Command(PackageResource, GetFunction, packageResourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageMidVersion);
        }

        /// <summary>
        /// Calls `set` on the `package` resource to request a specific version when a higher version is installed.
        /// </summary>
        [Test]
        public void Package_Set_SpecificVersionDowngrade()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                Version = DefaultPackageMidVersion,
            };

            var result = RunDSCv3Command(PackageResource, SetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);

            (PackageResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageMidVersion, ignoreLatest: true);

            AssertDiffState(diff, [ VersionPropertyName ]);

            // Call `get` to ensure the result
            PackageResourceData packageResourceDataForGet = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
            };

            result = RunDSCv3Command(PackageResource, GetFunction, packageResourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<PackageResourceData>(result.StdOut);
            AssertExistingPackageResourceData(output, DefaultPackageMidVersion);
        }

        /// <summary>
        /// Calls `export` on the `package` resource without providing any input.
        /// </summary>
        [Test]
        public void Package_Export_NoInput()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            var result = RunDSCv3Command(PackageResource, ExportFunction, " ");
            AssertSuccessfulResourceRun(ref result);

            List<PackageResourceData> output = GetOutputLinesAs<PackageResourceData>(result.StdOut);

            bool foundDefaultPackage = false;
            foreach (PackageResourceData item in output)
            {
                if (item.Identifier == DefaultPackageIdentifier)
                {
                    foundDefaultPackage = true;
                    Assert.IsNull(item.Version);
                    break;
                }
            }

            Assert.IsTrue(foundDefaultPackage);
        }

        /// <summary>
        /// Calls `export` on the `package` resource providing input to request that versions be included.
        /// </summary>
        [Test]
        public void Package_Export_RequestVersions()
        {
            var setupInstall = TestCommon.RunAICLICommand("install", $"--id {DefaultPackageIdentifier} --version {DefaultPackageLowVersion}");
            Assert.AreEqual(0, setupInstall.ExitCode);

            PackageResourceData packageResourceData = new PackageResourceData()
            {
                UseLatest = false,
            };

            var result = RunDSCv3Command(PackageResource, ExportFunction, packageResourceData, 300000);
            AssertSuccessfulResourceRun(ref result);

            List<PackageResourceData> output = GetOutputLinesAs<PackageResourceData>(result.StdOut);

            bool foundDefaultPackage = false;
            foreach (PackageResourceData item in output)
            {
                if (item.Identifier == DefaultPackageIdentifier)
                {
                    foundDefaultPackage = true;
                    Assert.AreEqual(DefaultPackageLowVersion, item.Version);
                }
                else
                {
                    Assert.IsNotNull(item.Version);
                    Assert.IsNotEmpty(item.Version);
                }
            }

            Assert.IsTrue(foundDefaultPackage);
        }

        private static void RemoveTestPackage()
        {
            PackageResourceData packageResourceData = new PackageResourceData()
            {
                Identifier = DefaultPackageIdentifier,
                Exist = false,
            };

            var result = RunDSCv3Command(PackageResource, SetFunction, packageResourceData);
            AssertSuccessfulResourceRun(ref result);
        }

        private static void AssertExistingPackageResourceData(PackageResourceData output, string version, bool ignoreLatest = false)
        {
            Assert.IsNotNull(output);
            Assert.True(output.Exist);
            Assert.AreEqual(DefaultPackageIdentifier, output.Identifier);
            Assert.AreEqual(version, output.Version);

            if (!ignoreLatest)
            {
                if (version == DefaultPackageHighVersion)
                {
                    Assert.True(output.UseLatest);
                }
                else
                {
                    Assert.False(output.UseLatest);
                }
            }
        }

        private class PackageResourceData
        {
            [JsonPropertyName(ExistPropertyName)]
            public bool? Exist { get; set; }

            [JsonPropertyName(InDesiredStatePropertyName)]
            public bool? InDesiredState { get; set; }

            [JsonPropertyName("id")]
            public string Identifier { get; set; }

            public string Source { get; set; }

            public string Version { get; set; }

            public string MatchOption { get; set; }

            public bool? UseLatest { get; set; }

            public string InstallMode { get; set; }

            public bool? AcceptAgreements { get; set; }
        }
    }
}
