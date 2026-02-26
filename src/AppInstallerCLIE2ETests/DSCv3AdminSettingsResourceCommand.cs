// -----------------------------------------------------------------------------
// <copyright file="DSCv3AdminSettingsResourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.Collections.Generic;
    using System.Text.Json;
    using System.Text.Json.Nodes;
    using System.Text.Json.Serialization;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `Configure` command tests.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Closing square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
    public class DSCv3AdminSettingsResourceCommand : DSCv3ResourceTestBase
    {
        private const string AdminSettingsResource = "admin-settings";
        private const string SettingsPropertyName = "settings";

        // Bool settings
        private const string BypassCertificatePinningForMicrosoftStore = "BypassCertificatePinningForMicrosoftStore";
        private const string InstallerHashOverride = "InstallerHashOverride";
        private const string LocalArchiveMalwareScanOverride = "LocalArchiveMalwareScanOverride";
        private const string LocalManifestFiles = "LocalManifestFiles";
        private const string ProxyCommandLineOptions = "ProxyCommandLineOptions";

        // String settings
        private const string DefaultProxy = "DefaultProxy";

        // Not a setting
        private const string NotAnAdminSettingName = "NotAnAdminSetting";

        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            EnsureTestResourcePresence();
        }

        /// <summary>
        /// Teardown done once after all the tests here.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTeardown()
        {
            ResetAllSettings();
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            ResetAllSettings();
            GroupPolicyHelper.DeleteExistingPolicies();
        }

        /// <summary>
        /// Calls `get` on the `admin-settings` resource with the value not present.
        /// </summary>
        /// <param name="function">The resource function to invoke.</param>
        [TestCase(GetFunction)]
        [TestCase(ExportFunction)]
        public void AdminSettings_Get(string function)
        {
            var result = RunDSCv3Command(AdminSettingsResource, function, null);
            AssertSuccessfulResourceRun(ref result);

            AdminSettingsResourceData output = GetSingleOutputLineAs<AdminSettingsResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.IsNotNull(output.Settings);
            Assert.IsTrue(output.Settings.ContainsKey(LocalManifestFiles));
            Assert.IsFalse(output.Settings.ContainsKey(DefaultProxy));
            Assert.IsFalse(output.Settings.ContainsKey(NotAnAdminSettingName));
        }

        /// <summary>
        /// Calls `test` on the `admin-settings` resource with a bool setting.
        /// </summary>
        /// <param name="settingName">The setting to test.</param>
        [TestCase(BypassCertificatePinningForMicrosoftStore)]
        [TestCase(InstallerHashOverride)]
        [TestCase(LocalArchiveMalwareScanOverride)]
        [TestCase(LocalManifestFiles)]
        [TestCase(ProxyCommandLineOptions)]
        public void AdminSettings_Test_BoolSetting(string settingName)
        {
            AdminSettingsResourceData resourceData = new AdminSettingsResourceData() { Settings = new JsonObject() };

            resourceData.Settings[settingName] = true;
            var result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfBoolSetting(ref result, settingName, false, true);

            resourceData.Settings[settingName] = false;
            result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfBoolSetting(ref result, settingName, false, false);

            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("settings", $"--enable {settingName}").ExitCode);

            resourceData.Settings[settingName] = false;
            result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfBoolSetting(ref result, settingName, true, false);

            resourceData.Settings[settingName] = true;
            result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfBoolSetting(ref result, settingName, true, true);
        }

        /// <summary>
        /// Calls `test` on the `admin-settings` resource with a string setting.
        /// </summary>
        /// <param name="settingName">The setting to test.</param>
        [TestCase(DefaultProxy)]
        public void AdminSettings_Test_StringSetting(string settingName)
        {
            const string testValue = "A string to test";
            const string differentTestValue = "A different value";

            AdminSettingsResourceData resourceData = new AdminSettingsResourceData() { Settings = new JsonObject() };

            resourceData.Settings[settingName] = null;
            var result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfStringSetting(ref result, settingName, null, null);

            resourceData.Settings[settingName] = testValue;
            result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfStringSetting(ref result, settingName, null, testValue);

            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("settings set", $"{settingName} \"{testValue}\"").ExitCode);

            resourceData.Settings[settingName] = null;
            result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfStringSetting(ref result, settingName, testValue, null);

            resourceData.Settings[settingName] = testValue;
            result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfStringSetting(ref result, settingName, testValue, testValue);

            resourceData.Settings[settingName] = differentTestValue;
            result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertTestOfStringSetting(ref result, settingName, testValue, differentTestValue);
        }

        /// <summary>
        /// Calls `test` on the `admin-settings` resource with a complex input.
        /// </summary>
        [Test]
        public void AdminSettings_Test_Complex()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("settings", $"--enable {LocalArchiveMalwareScanOverride}").ExitCode);
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("settings", $"--enable {LocalManifestFiles}").ExitCode);

            AdminSettingsResourceData resourceData = new AdminSettingsResourceData() { Settings = new JsonObject() };
            resourceData.Settings[LocalArchiveMalwareScanOverride] = true;
            resourceData.Settings[BypassCertificatePinningForMicrosoftStore] = false;
            resourceData.Settings[DefaultProxy] = null;

            var result = RunDSCv3Command(AdminSettingsResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (AdminSettingsResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<AdminSettingsResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.IsTrue(output.InDesiredState);
            Assert.IsNotNull(output.Settings);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `set` on the `admin-settings` resource with a bool setting.
        /// </summary>
        /// <param name="settingName">The setting to test.</param>
        [TestCase(BypassCertificatePinningForMicrosoftStore)]
        [TestCase(InstallerHashOverride)]
        [TestCase(LocalArchiveMalwareScanOverride)]
        [TestCase(LocalManifestFiles)]
        [TestCase(ProxyCommandLineOptions)]
        public void AdminSettings_Set_BoolSetting(string settingName)
        {
            AdminSettingsResourceData resourceData = new AdminSettingsResourceData() { Settings = new JsonObject() };

            resourceData.Settings[settingName] = true;
            var result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfBoolSetting(ref result, settingName, false, true);

            result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfBoolSetting(ref result, settingName, true, true);

            resourceData.Settings[settingName] = false;
            result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfBoolSetting(ref result, settingName, true, false);

            result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfBoolSetting(ref result, settingName, false, false);
        }

        /// <summary>
        /// Calls `set` on the `admin-settings` resource with a string setting.
        /// </summary>
        /// <param name="settingName">The setting to test.</param>
        [TestCase(DefaultProxy)]
        public void AdminSettings_Set_StringSetting(string settingName)
        {
            const string testValue = "A string to test";
            const string differentTestValue = "A different value";

            AdminSettingsResourceData resourceData = new AdminSettingsResourceData() { Settings = new JsonObject() };

            resourceData.Settings[settingName] = null;
            var result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfStringSetting(ref result, settingName, null, null);

            resourceData.Settings[settingName] = testValue;
            result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfStringSetting(ref result, settingName, null, testValue);

            resourceData.Settings[settingName] = testValue;
            result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfStringSetting(ref result, settingName, testValue, testValue);

            resourceData.Settings[settingName] = differentTestValue;
            result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfStringSetting(ref result, settingName, testValue, differentTestValue);

            resourceData.Settings[settingName] = null;
            result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSetOfStringSetting(ref result, settingName, differentTestValue, null);
        }

        /// <summary>
        /// Calls `set` on the `admin-settings` resource with a complex input.
        /// </summary>
        [Test]
        public void AdminSettings_Set_Complex()
        {
            const string testValue = "A string to test";

            AdminSettingsResourceData resourceData = new AdminSettingsResourceData() { Settings = new JsonObject() };
            resourceData.Settings[LocalArchiveMalwareScanOverride] = true;
            resourceData.Settings[BypassCertificatePinningForMicrosoftStore] = false;
            resourceData.Settings[DefaultProxy] = testValue;

            var result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (AdminSettingsResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<AdminSettingsResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.IsNotNull(output.Settings);
            Assert.AreEqual(JsonValueKind.True, output.Settings[LocalArchiveMalwareScanOverride].AsValue().GetValueKind());
            Assert.AreEqual(JsonValueKind.False, output.Settings[BypassCertificatePinningForMicrosoftStore].AsValue().GetValueKind());
            Assert.AreEqual(testValue, output.Settings[DefaultProxy].AsValue().GetValue<string>());

            AssertDiffState(diff, [ SettingsPropertyName ]);
        }

        /// <summary>
        /// Calls `set` on the `admin-settings` resource attempting to change a setting with group policy enabled.
        /// </summary>
        [Test]
        public void AdminSettings_Set_GroupPolicyBlocked()
        {
            GroupPolicyHelper.EnableHashOverride.Disable();

            AdminSettingsResourceData resourceData = new AdminSettingsResourceData() { Settings = new JsonObject() };
            resourceData.Settings[InstallerHashOverride] = true;

            var result = RunDSCv3Command(AdminSettingsResource, SetFunction, resourceData);
            Assert.AreEqual(Constants.ErrorCode.ERROR_BLOCKED_BY_POLICY, result.ExitCode);
        }

        private static void ResetAllSettings()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("settings reset", "--all").ExitCode);
        }

        private static void AssertTestOfBoolSetting(ref TestCommon.RunCommandResult result, string settingName, bool expectedState, bool testState)
        {
            AssertSuccessfulResourceRun(ref result);

            bool inDesiredState = expectedState == testState;

            (AdminSettingsResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<AdminSettingsResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.AreEqual(inDesiredState, output.InDesiredState);
            Assert.IsNotNull(output.Settings);
            Assert.IsTrue(output.Settings.ContainsKey(settingName));
            Assert.AreEqual(expectedState ? JsonValueKind.True : JsonValueKind.False, output.Settings[settingName].AsValue().GetValueKind());

            AssertDiffState(diff, inDesiredState ? [] : [ SettingsPropertyName ]);
        }

        private static void AssertSetOfBoolSetting(ref TestCommon.RunCommandResult result, string settingName, bool previousState, bool desiredState)
        {
            AssertSuccessfulResourceRun(ref result);

            bool inDesiredState = previousState == desiredState;

            (AdminSettingsResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<AdminSettingsResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.IsNotNull(output.Settings);
            Assert.IsTrue(output.Settings.ContainsKey(settingName));
            Assert.AreEqual(desiredState ? JsonValueKind.True : JsonValueKind.False, output.Settings[settingName].AsValue().GetValueKind());

            AssertDiffState(diff, inDesiredState ? [] : [ SettingsPropertyName ]);
        }

        private static void AssertTestOfStringSetting(ref TestCommon.RunCommandResult result, string settingName, string expectedState, string testState)
        {
            AssertSuccessfulResourceRun(ref result);

            bool inDesiredState = expectedState == testState;

            (AdminSettingsResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<AdminSettingsResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.AreEqual(inDesiredState, output.InDesiredState);
            Assert.IsNotNull(output.Settings);
            if (expectedState != null)
            {
                Assert.IsTrue(output.Settings.ContainsKey(settingName));
                Assert.AreEqual(expectedState, output.Settings[settingName].AsValue().GetValue<string>());
            }
            else
            {
                Assert.IsFalse(output.Settings.ContainsKey(settingName));
            }

            AssertDiffState(diff, inDesiredState ? [] : [SettingsPropertyName]);
        }

        private static void AssertSetOfStringSetting(ref TestCommon.RunCommandResult result, string settingName, string previousState, string desiredState)
        {
            AssertSuccessfulResourceRun(ref result);

            bool inDesiredState = previousState == desiredState;

            (AdminSettingsResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<AdminSettingsResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.IsNotNull(output.Settings);
            if (desiredState != null)
            {
                Assert.IsTrue(output.Settings.ContainsKey(settingName));
                Assert.AreEqual(desiredState, output.Settings[settingName].AsValue().GetValue<string>());
            }
            else
            {
                Assert.IsFalse(output.Settings.ContainsKey(settingName));
            }

            AssertDiffState(diff, inDesiredState ? [] : [SettingsPropertyName]);
        }

        private class AdminSettingsResourceData
        {
            [JsonPropertyName(InDesiredStatePropertyName)]
            public bool? InDesiredState { get; set; }

            public JsonObject Settings { get; set; }
        }
    }
}
