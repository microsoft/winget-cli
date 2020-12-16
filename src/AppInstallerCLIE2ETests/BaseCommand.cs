// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Threading;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;
    using NUnit.Framework;

    public class BaseCommand
    {
        public string SettingsJsonFilePath => TestCommon.PackagedContext ?
            @"Packages\WinGetDevCLI_8wekyb3d8bbwe\LocalState\settings.json" :
            @"Microsoft\WinGet\Settings\settings.json";
        public readonly string LocalAppData = "LocalAppData";

        [OneTimeSetUp]
        public void BaseSetup()
        {
            ResetTestSource();
        }

        [OneTimeTearDown]
        public void BaseTeardown()
        {
            TestCommon.RunAICLICommand("source reset", "--force");
        }

        public void ResetTestSource()
        {
            TestCommon.RunAICLICommand("source reset", "--force");
            TestCommon.RunAICLICommand("source remove", Constants.DefaultSourceName);
            TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
            Thread.Sleep(5000);
        }

        public void ConfigureFeature(string featureName, bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, SettingsJsonFilePath)));
            JObject experimentalFeatures = (JObject)settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;

            File.WriteAllText(Path.Combine(localAppDataPath, SettingsJsonFilePath), settingsJson.ToString());
        }

        public void InitializeAllFeatures(bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(LocalAppData);

            var settingsJson = new
            {
                experimentalFeatures = new
                {
                    experimentalArg = status,
                    experimentalCmd = status,
                    experimentalMSStore = status,
                    list = status,
                    upgrade = status,
                    uninstall = status,
                }
            };

            var serializedSettingsJson = JsonConvert.SerializeObject(settingsJson, Formatting.Indented);
            File.WriteAllText(Path.Combine(localAppDataPath, SettingsJsonFilePath), serializedSettingsJson);
        }
    }
}
