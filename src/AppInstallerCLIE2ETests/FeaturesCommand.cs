// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using NUnit.Framework;

    public class FeaturesCommand
    {
        private const string SettingsJsonFilePath = @"Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\settings.json";
        private const string LocalAppData = "LocalAppData";

        [SetUp]
        public void Setup()
        {
            InitializeAllFeatures(false);
        }

        [TearDown]
        public void TearDown()
        {
            InitializeAllFeatures(false);
        }

        [Test]
        public void DisplayFeatures()
        {
            var result = TestCommon.RunAICLICommand("features", "");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Command Sample"));
            Assert.True(result.StdOut.Contains("Argument Sample"));
            Assert.True(result.StdOut.Contains("Microsoft Store Support"));
            Assert.False(result.StdOut.Contains("Enabled"));
        }

        [Test]
        public void EnableExperimentalFeatures()
        {
            ConfigureFeature("experimentalArg", true);
            ConfigureFeature("experimentalCmd", true);
            ConfigureFeature("experimentalMSStore", true);
            var result = TestCommon.RunAICLICommand("features", "");
            Assert.True(result.StdOut.Contains("Enabled"));
        }

        private void ConfigureFeature(string featureName, bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, SettingsJsonFilePath)));
            JObject experimentalFeatures = (JObject)settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;

            File.WriteAllText(Path.Combine(localAppDataPath, SettingsJsonFilePath), settingsJson.ToString());
        }

        private void InitializeAllFeatures(bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(LocalAppData);

            var settingsJson = new
            {
                experimentalFeatures = new
                {
                    experimentalArg = status,
                    experimentalCmd = status,
                    experimentalMSStore = status
                }
            };

            var serializedSettingsJson = JsonConvert.SerializeObject(settingsJson, Formatting.Indented);
            File.WriteAllText(Path.Combine(localAppDataPath, SettingsJsonFilePath), serializedSettingsJson);
        }
    }
}
