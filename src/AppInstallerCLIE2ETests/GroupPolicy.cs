// -----------------------------------------------------------------------------
// <copyright file="GroupPolicy.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Tests for enforcement of Group Policy.
    /// Behavior is better tested in the unit tests; these tests mostly ensure match between the code and the definition.
    /// </summary>
    public class GroupPolicy : BaseCommand
    {
        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            WinGetSettingsHelper.InitializeAllFeatures(false);
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            WinGetSettingsHelper.InitializeAllFeatures(false);
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Test winget search is disabled by policy.
        /// </summary>
        [Test]
        public void PolicyEnableWinget()
        {
            GroupPolicyHelper.EnableWinget.Disable();
            var result = TestCommon.RunAICLICommand("search", "foo");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);

            // Scenario if Policy WinGet is disabled but Policy EnableWindowsPackageManagerCommandLineInterfaces is Enabled.
            GroupPolicyHelper.EnableWinGetCommandLineInterfaces.Enable();
            result = TestCommon.RunAICLICommand("search", "foo");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);

            // Scenario if Policy WinGet is disabled but Policy EnableWindowsPackageManagerCommandLineInterfaces is Not-Configured.
            GroupPolicyHelper.EnableWinGetCommandLineInterfaces.SetNotConfigured();
            result = TestCommon.RunAICLICommand("search", "foo");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);

            // Scenario if Policy WinGet is enabled but Policy EnableWindowsPackageManagerCommandLineInterfaces is disabled.
            GroupPolicyHelper.EnableWinget.Enable();
            GroupPolicyHelper.EnableWinGetCommandLineInterfaces.Disable();
            result = TestCommon.RunAICLICommand("search", "foo");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);

            // Scenario if Policy WinGet is Not-Configured  but Policy EnableWindowsPackageManagerCommandLineInterfaces is disabled.
            GroupPolicyHelper.EnableWinget.SetNotConfigured();
            GroupPolicyHelper.EnableWinGetCommandLineInterfaces.Disable();
            result = TestCommon.RunAICLICommand("search", "foo");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        /// <summary>
        /// Test winget settings is disable by policy.
        /// </summary>
        [Test]
        public void EnableSettings()
        {
            GroupPolicyHelper.EnableSettings.Disable();
            var result = TestCommon.RunAICLICommand("settings", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        /// <summary>
        /// Test experimental features policy.
        /// </summary>
        [Test]
        public void EnableExperimentalFeatures()
        {
            WinGetSettingsHelper.ConfigureFeature("experimentalCmd", true);
            var result = TestCommon.RunAICLICommand("experimental", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            // An experimental feature disabled by Group Policy behaves the same as one that is not enabled.
            // The expected result is a command line error as the argument validation rejects this.
            GroupPolicyHelper.EnableExperimentalFeatures.Disable();
            result = TestCommon.RunAICLICommand("experimental", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS, result.ExitCode);
        }

        /// <summary>
        /// Test install via manifest is disabled by policy.
        /// </summary>
        [Test]
        public void EnableLocalManifests()
        {
            GroupPolicyHelper.EnableLocalManifests.Disable();
            var result = TestCommon.RunAICLICommand("install", $"-m {TestCommon.GetTestDataFile(@"Manifests\TestExeInstaller.yaml")}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        /// <summary>
        /// Test install without checking the hash is disabled by policy.
        /// </summary>
        [Test]
        public void EnableHashOverride()
        {
            GroupPolicyHelper.EnableHashOverride.Disable();
            var result = TestCommon.RunAICLICommand("install", "AnyPackage --ignore-security-hash");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        /// <summary>
        /// Test install ignoring the malware scan is disabled by policy.
        /// </summary>
        [Test]
        public void EnableIgnoreLocalArchiveMalwareScanOverride()
        {
            GroupPolicyHelper.EnableLocalArchiveMalwareScanOverride.Disable();
            var result = TestCommon.RunAICLICommand("install", "AnyPackage --ignore-local-archive-malware-scan");

            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        /// <summary>
        /// Test winget source is enabled by policy.
        /// </summary>
        [Test]
        public void EnableDefaultSource()
        {
            // Default sources are disabled during setup so they are missing.
            var result = TestCommon.RunAICLICommand("source list", "winget");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);

            GroupPolicyHelper.EnableDefaultSource.Enable();
            result = TestCommon.RunAICLICommand("source list", "winget");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
        }

        /// <summary>
        /// Test store source is enabled by policy.
        /// </summary>
        [Test]
        public void EnableMicrosoftStoreSource()
        {
            // Default sources are disabled during setup so they are missing.
            var result = TestCommon.RunAICLICommand("source list", "msstore");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);

            GroupPolicyHelper.EnableMicrosoftStoreSource.Enable();
            result = TestCommon.RunAICLICommand("source list", "msstore");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
        }

        /// <summary>
        /// Test additional sources are enabled by policy.
        /// </summary>
        [Test]
        public void EnableAdditionalSources()
        {
            // Remove the test source, then add it with policy.
            TestCommon.RunAICLICommand("source remove", "TestSource");
            var result = TestCommon.RunAICLICommand("source list", "TestSource");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);

            GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new string[]
            {
                "{\"Arg\":\"https://localhost:5001/TestKit\",\"Data\":\"WingetE2E.Tests_8wekyb3d8bbwe\",\"Identifier\":\"WingetE2E.Tests_8wekyb3d8bbwe\",\"Name\":\"TestSource\",\"Type\":\"Microsoft.PreIndexed.Package\"}",
            });

            result = TestCommon.RunAICLICommand("source list", "TestSource");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
        }

        /// <summary>
        /// Test additional sources with trust levels and explicit are enabled by policy.
        /// </summary>
        [Test]
        public void EnableAdditionalSources_TrustLevel_Explicit()
        {
            // Remove the test source, then add it with policy.
            TestCommon.RunAICLICommand("source remove", "TestSource");
            var result = TestCommon.RunAICLICommand("source list", "TestSource");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);

            GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new string[]
            {
                "{\"Arg\":\"https://localhost:5001/TestKit\",\"Data\":\"WingetE2E.Tests_8wekyb3d8bbwe\",\"Identifier\":\"WingetE2E.Tests_8wekyb3d8bbwe\",\"Name\":\"TestSource\",\"Type\":\"Microsoft.PreIndexed.Package\",\"TrustLevel\":[\"Trusted\"],\"Explicit\":true}",
            });

            result = TestCommon.RunAICLICommand("source list", "TestSource");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Trust Level"));
            Assert.True(result.StdOut.Contains("Trusted"));

            var searchResult = TestCommon.RunAICLICommand("search", "TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_SOURCES_DEFINED, searchResult.ExitCode);
            Assert.True(searchResult.StdOut.Contains("No sources defined; add one with 'source add' or reset to defaults with 'source reset'"));
        }

        /// <summary>
        /// Test enable allowed sources.
        /// </summary>
        [Test]
        public void EnableAllowedSources()
        {
            // Try listing the test source. We should only see it if it is allowed.
            // With allowed sources disabled:
            GroupPolicyHelper.EnableAllowedSources.Disable();
            var result = TestCommon.RunAICLICommand("source list", "TestSource");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);

            // With allowed sources enabled, but not listing the test source:
            GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new string[]
            {
                "{\"Arg\":\"An argument\",\"Data\":\"Some data\",\"Identifier\":\"Test id\",\"Name\":\"NotTestSource\",\"Type\":\"Microsoft.PreIndexed.Package\"}",
            });

            result = TestCommon.RunAICLICommand("source list", "TestSource");
            Assert.AreEqual(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST, result.ExitCode);

            // With the test source allowed:
            GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new string[]
            {
                "{\"Arg\":\"https://localhost:5001/TestKit\",\"Data\":\"WingetE2E.Tests_8wekyb3d8bbwe\",\"Identifier\":\"WingetE2E.Tests_8wekyb3d8bbwe\",\"Name\":\"TestSource\",\"Type\":\"Microsoft.PreIndexed.Package\"}",
            });

            result = TestCommon.RunAICLICommand("source list", "TestSource");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
        }

        /// <summary>
        /// Tests source auto update policy.
        /// </summary>
        [Test]
        public void SourceAutoUpdateInterval()
        {
            // Test this policy by inspecting the result of --info
            GroupPolicyHelper.SourceAutoUpdateInterval.SetEnabledValue(123);
            var result = TestCommon.RunAICLICommand(string.Empty, "--info");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("Source Auto Update Interval In Minutes 123"));
        }

        /// <summary>
        /// Test configuration is disabled by policy.
        /// </summary>
        [Test]
        public void EnableConfiguration()
        {
            GroupPolicyHelper.EnableConfiguration.Disable();
            var result = TestCommon.RunAICLICommand("configure", TestCommon.GetTestDataFile("Configuration\\ShowDetails_TestRepo.yml"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);

            result = TestCommon.RunAICLICommand("configure show", TestCommon.GetTestDataFile("Configuration\\ShowDetails_TestRepo.yml"));
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }
    }
}
