// -----------------------------------------------------------------------------
// <copyright file="DSCv3UserSettingsFileResourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests;

using AppInstallerCLIE2ETests.Helpers;
using NUnit.Framework;
using System.Collections.Generic;
using System.IO;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;

/// <summary>
/// `Configure` command tests.
/// </summary>
[System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
[System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Closing square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
public class DSCv3UserSettingsFileResourceCommand : DSCv3ResourceTestBase
{
    private const string UserSettingsFileResource = "user-settings-file";
    private const string SettingsPropertyName = "settings";
    private const string ActionPropertyValueFull = "Full";
    private const string ActionPropertyValuePartial = "Partial";
    private const string SettingsMock = "mock";
    private const string SettingsMockObject = "mockObject";
    private const string SettingsMockNested = "mockNested";

    /// <summary>
    /// Setup done once before all the tests here.
    /// </summary>
    [OneTimeSetUp]
    public void OneTimeSetup()
    {
        TestCommon.SetupTestSource();
        EnsureTestResourcePresence();
    }

    /// <summary>
    /// Teardown done once after all the tests here.
    /// </summary>
    [OneTimeTearDown]
    public void OneTimeTeardown()
    {
        WinGetSettingsHelper.InitializeWingetSettings();
    }

    /// <summary>
    /// Set up.
    /// </summary>
    [SetUp]
    public void Setup()
    {
        // Reset the settings to default before each test.
        WinGetSettingsHelper.InitializeWingetSettings();
        WinGetSettingsHelper.ConfigureFeature("dsc3", true);
    }

    /// <summary>
    /// Calls `get` on the `user-settings-file` resource.
    /// </summary>
    [Test]
    public void UserSettingsFile_Get()
    {
        var expected = GetCurrentUserSettings();
        var getOutput = Get(new ());

        Assert.IsNotNull(getOutput);
        Assert.IsNull(getOutput.Action);
        AssertSettingsAreEqual(expected, getOutput.Settings);
    }

    /// <summary>
    /// Calls `set` on the `user-settings-file` resource with no diff.
    /// </summary>
    /// <param name="action">The action value.</param>
    [Test]
    [TestCase(ActionPropertyValueFull)]
    [TestCase(ActionPropertyValuePartial)]
    public void UserSettingsFile_Set_NoDiff(string action)
    {
        var setSettings = GetSettingsArg(action);

        (var setOutput, var setDiff) = Set(new () { Action = action, Settings = setSettings });

        var expected = GetCurrentUserSettings();

        Assert.IsNotNull(setOutput);
        Assert.AreEqual(action, setOutput.Action);
        AssertSettingsAreEqual(expected, setOutput.Settings);
        AssertDiffState(setDiff, []);
    }

    /// <summary>
    /// Calls `set` on the `user-settings-file` resource to add fields.
    /// </summary>
    /// <param name="action">The action value.</param>
    [Test]
    [TestCase(ActionPropertyValueFull)]
    [TestCase(ActionPropertyValuePartial)]
    public void UserSettingsFile_Set_AddFields(string action)
    {
        // Call `set` to add mock properties to the settings
        var setSettings = GetSettingsArg(action);
        AddOrModifyMockProperties(setSettings, "mock");

        (var setOutput, var setDiff) = Set(new () { Action = action, Settings = setSettings });

        var expected = GetCurrentUserSettings();

        // Assert that the settings are added
        Assert.IsNotNull(setOutput);
        Assert.AreEqual(action, setOutput.Action);
        AssertMockProperties(setOutput.Settings, "mock");
        AssertSettingsAreEqual(expected, setOutput.Settings);
        AssertDiffState(setDiff, [ SettingsPropertyName ]);
    }

    /// <summary>
    /// Calls `set` on the `user-settings-file` resource to ensure action is partial by default.
    /// </summary>
    [Test]
    public void UserSettingsFile_Set_ActionIsPartialByDefault()
    {
        // Call `set` to add mock properties to the settings
        var setSettings = GetSettingsArg(ActionPropertyValuePartial);
        AddOrModifyMockProperties(setSettings, "mock");

        var expected = GetCurrentUserSettings();
        AddOrModifyMockProperties(expected, "mock");

        (var setOutput, var setDiff) = Set(new () { Settings = setSettings });

        // Assert that the settings are added
        Assert.IsNotNull(setOutput);
        Assert.AreEqual(setOutput.Action, ActionPropertyValuePartial);
        AssertMockProperties(setOutput.Settings, "mock");
        AssertSettingsAreEqual(expected, setOutput.Settings);
        AssertDiffState(setDiff, [ SettingsPropertyName ]);
    }

    /// <summary>
    /// Calls `set` on the `user-settings-file` resource to update fields.
    /// </summary>
    /// <param name="action">The action value.</param>
    [Test]
    [TestCase(ActionPropertyValueFull)]
    [TestCase(ActionPropertyValuePartial)]
    public void UserSettingsFile_Set_UpdateFields(string action)
    {
        // Add mock properties to the settings
        var set1Settings = new JsonObject();
        AddOrModifyMockProperties(set1Settings, "mock_old");
        Set(new () { Action = ActionPropertyValuePartial, Settings = set1Settings });

        // Call `set` to update the settings
        var set2Settings = GetSettingsArg(action);
        AddOrModifyMockProperties(set2Settings, "mock_new");

        (var setOutput, var setDiff) = Set(new () { Action = action, Settings = set2Settings });

        var expected = GetCurrentUserSettings();

        // Assert that the settings are updated
        Assert.IsNotNull(setOutput);
        Assert.AreEqual(action, setOutput.Action);
        AssertMockProperties(setOutput.Settings, "mock_new");
        AssertSettingsAreEqual(expected, setOutput.Settings);
        AssertDiffState(setDiff, [ SettingsPropertyName ]);
    }

    /// <summary>
    /// Calls `test` on the `user-settings-file` resource to check if the settings are in desired state.
    /// </summary>
    /// <param name="action">The action value.</param>
    [Test]
    [TestCase(ActionPropertyValueFull)]
    [TestCase(ActionPropertyValuePartial)]
    public void UserSettingsFile_Test_InDesiredState(string action)
    {
        // Add mock properties to the settings
        var setSettings = new JsonObject();
        AddOrModifyMockProperties(setSettings, "mock");
        Set(new () { Action = ActionPropertyValuePartial, Settings = setSettings });

        // Call `test` to check the settings
        var testSettings = GetSettingsArg(action);
        AddOrModifyMockProperties(testSettings, "mock");

        (var testOutput, var testDiff) = Test(new () { Action = action, Settings = testSettings });

        var expected = GetCurrentUserSettings();

        // Assert that the settings are in desired state
        Assert.IsNotNull(testOutput);
        Assert.AreEqual(action, testOutput.Action);
        AssertMockProperties(testOutput.Settings, "mock");
        AssertSettingsAreEqual(expected, testOutput.Settings);
        Assert.IsTrue(testOutput.InDesiredState);
        AssertDiffState(testDiff, []);
    }

    /// <summary>
    /// Calls `test` on the `user-settings-file` resource to check if the settings are not in desired state.
    /// </summary>
    /// <param name="action">The action value.</param>
    [Test]
    [TestCase(ActionPropertyValueFull)]
    [TestCase(ActionPropertyValuePartial)]
    public void UserSettingsFile_Test_NotInDesiredState(string action)
    {
        // Add mock properties to the settings
        var setSettings = new JsonObject();
        AddOrModifyMockProperties(setSettings, "mock_set");
        Set(new () { Action = ActionPropertyValuePartial, Settings = setSettings });

        // Call `test` to check the settings
        var testSettings = GetSettingsArg(action);
        AddOrModifyMockProperties(testSettings, "mock_test");

        (var testOutput, var testDiff) = Test(new () { Action = action, Settings = testSettings });

        var expected = GetCurrentUserSettings();

        // Assert that the settings are not in desired state
        Assert.IsNotNull(testOutput);
        Assert.AreEqual(action, testOutput.Action);
        AssertMockProperties(testOutput.Settings, "mock_set");
        AssertSettingsAreEqual(expected, testOutput.Settings);
        Assert.IsFalse(testOutput.InDesiredState);
        AssertDiffState(testDiff, [ SettingsPropertyName ]);
    }

    /// <summary>
    /// Calls `export` on the `user-settings-file` resource to export the settings.
    /// </summary>
    [Test]
    public void UserSettingsFile_Export()
    {
        var expected = GetCurrentUserSettings();
        var exportOutput = Export(new ());

        Assert.IsNotNull(exportOutput);
        Assert.IsNull(exportOutput.Action);
        AssertSettingsAreEqual(expected, exportOutput.Settings);
    }

    /// <summary>
    /// Calls `get` on the `user-settings-file` resource.
    /// </summary>
    /// <param name="resourceData">The input resource data.</param>
    /// <returns>The output resource data.</returns>
    private static UserSettingsFileResourceData Get(UserSettingsFileResourceData resourceData)
    {
        var result = RunDSCv3Command(UserSettingsFileResource, GetFunction, resourceData);
        AssertSuccessfulResourceRun(ref result);
        return GetSingleOutputLineAs<UserSettingsFileResourceData>(result.StdOut);
    }

    /// <summary>
    /// Calls `set` on the `user-settings-file` resource.
    /// </summary>
    /// <param name="resourceData">The input resource data.</param>
    /// <returns>The output resource data and the diff.</returns>
    private static (UserSettingsFileResourceData, List<string>) Set(UserSettingsFileResourceData resourceData)
    {
        var result = RunDSCv3Command(UserSettingsFileResource, SetFunction, resourceData);
        AssertSuccessfulResourceRun(ref result);
        return GetSingleOutputLineAndDiffAs<UserSettingsFileResourceData>(result.StdOut);
    }

    /// <summary>
    /// Calls `test` on the `user-settings-file` resource.
    /// </summary>
    /// <param name="resourceData">The input resource data.</param>
    /// <returns>The output resource data and the diff.</returns>
    private static (UserSettingsFileResourceData, List<string>) Test(UserSettingsFileResourceData resourceData)
    {
        var result = RunDSCv3Command(UserSettingsFileResource, TestFunction, resourceData);
        AssertSuccessfulResourceRun(ref result);
        return GetSingleOutputLineAndDiffAs<UserSettingsFileResourceData>(result.StdOut);
    }

    /// <summary>
    /// Calls `export` on the `user-settings-file` resource.
    /// </summary>
    /// <param name="resourceData">The input resource data.</param>
    /// <returns>The output resource data.</returns>
    private static UserSettingsFileResourceData Export(UserSettingsFileResourceData resourceData)
    {
        var result = RunDSCv3Command(UserSettingsFileResource, ExportFunction, resourceData);
        AssertSuccessfulResourceRun(ref result);
        return GetSingleOutputLineAs<UserSettingsFileResourceData>(result.StdOut);
    }

    /// <summary>
    /// Gets the current user settings from the settings file.
    /// </summary>
    /// <returns>The current user settings as a JsonObject.</returns>
    private static JsonObject GetCurrentUserSettings()
    {
        var settingsContent = File.ReadAllText(WinGetSettingsHelper.GetUserSettingsPath());
        return JsonNode.Parse(settingsContent).AsObject();
    }

    /// <summary>
    /// Adds or modifies mock properties in the settings.
    /// </summary>
    /// <param name="settings">Target settings.</param>
    /// <param name="value">The mock value.</param>
    private static void AddOrModifyMockProperties(JsonObject settings, string value)
    {
        settings[SettingsMock] = value;
        settings[SettingsMockObject] ??= new JsonObject();
        settings[SettingsMockObject][SettingsMockNested] = value;
    }

    /// <summary>
    /// Asserts that the settings contain the expected mock properties.
    /// </summary>
    /// <param name="settings">Target settings.</param>
    /// <param name="value">The expected mock value.</param>
    private static void AssertMockProperties(JsonObject settings, string value)
    {
        Assert.IsNotNull(settings);
        Assert.IsTrue(settings.ContainsKey(SettingsMock));
        Assert.AreEqual(settings[SettingsMock].ToString(), value);
        Assert.IsTrue(settings.ContainsKey(SettingsMockObject));
        Assert.IsTrue(settings[SettingsMockObject].AsObject().ContainsKey(SettingsMockNested));
        Assert.AreEqual(settings[SettingsMockObject][SettingsMockNested].ToString(), value);
    }

    /// <summary>
    /// Asserts that the diff state is as expected.
    /// </summary>
    /// <param name="expected">The expected settings.</param>
    /// <param name="actual">The actual settings.</param>
    private static void AssertSettingsAreEqual(JsonObject expected, JsonObject actual)
    {
        Assert.IsTrue(JsonNode.DeepEquals(expected, actual));
    }

    /// <summary>
    /// Gets the settings argument based on the action.
    /// </summary>
    /// <param name="action">The action value.</param>
    /// <returns>The settings argument as a JsonObject.</returns>
    private static JsonObject GetSettingsArg(string action) => action == ActionPropertyValueFull ? GetCurrentUserSettings() : new ();

    private class UserSettingsFileResourceData
    {
        [JsonPropertyName(InDesiredStatePropertyName)]
        public bool? InDesiredState { get; set; }

        [JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingDefault)]
        public string Action { get; set; }

        public JsonObject Settings { get; set; }
    }
}
