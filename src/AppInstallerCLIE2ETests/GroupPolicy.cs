// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using NUnit.Framework;

namespace AppInstallerCLIE2ETests
{
    /// <summary>
    /// Tests for enforcement of Group Policy.
    /// Behavior is better tested in the unit tests; these tests mostly ensure match between the code and the definition.
    /// </summary>
    public class GroupPolicy : BaseCommand
    {
        [SetUp]
        public void Setup()
        {
            InitializeAllFeatures(false);
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        [TearDown]
        public void TearDown()
        {
            InitializeAllFeatures(false);
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        [Test]
        public void PolicyEnableWinget()
        {
            GroupPolicyHelper.EnableWinget.Disable();
            var result = TestCommon.RunAICLICommand("search", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        [Test]
        public void EnableSettings()
        {
            GroupPolicyHelper.EnableSettings.Disable();
            var result = TestCommon.RunAICLICommand("settings", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        [Test]
        public void EnableExperimentalFeatures()
        {
            ConfigureFeature("experimentalCmd", true);
            var result = TestCommon.RunAICLICommand("experimental", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);

            // An experimental feature disabled by Group Policy behaves the same as one that is not enabled.
            // The expected result is a command line error as the argument validation rejects this.
            GroupPolicyHelper.EnableExperimentalFeatures.Disable();
            result = TestCommon.RunAICLICommand("experimental", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS, result.ExitCode);
        }

        [Test]
        public void EnableLocalManifests()
        {
            GroupPolicyHelper.EnableLocalManifests.Disable();
            var result = TestCommon.RunAICLICommand("install", $"-m {TestCommon.GetTestDataFile(@"Manifests\TestExeInstaller.yaml")}");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        [Test]
        public void EnableHashOverride()
        {
            GroupPolicyHelper.EnableHashOverride.Disable();
            var result = TestCommon.RunAICLICommand("install", "AnyPackage --force");
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

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

        [Test]
        public void SourceAutoUpdateInterval()
        {
            // Test this policy by inspecting the result of --info
            GroupPolicyHelper.SourceAutoUpdateInterval.SetEnabledValue(123);
            var result = TestCommon.RunAICLICommand(string.Empty, "--info");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.IsTrue(result.StdOut.Contains("Source Auto Update Interval In Minutes 123"));
        }
    }
}
