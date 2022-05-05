// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Threading;
    using Newtonsoft.Json.Linq;
    using NUnit.Framework;

    public class BaseCommand
    {
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
            TestCommon.RunAICLICommand("source remove", Constants.DefaultWingetSourceName);
            TestCommon.RunAICLICommand("source remove", Constants.DefaultMSStoreSourceName);
            TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
            Thread.Sleep(2000);
        }

        public void ConfigureFeature(string featureName, bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(Constants.LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath)));
            JObject experimentalFeatures = (JObject)settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;

            File.WriteAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath), settingsJson.ToString());
        }

        public void InitializeAllFeatures(bool status)
        {
            ConfigureFeature("experimentalArg", status);
            ConfigureFeature("experimentalCmd", status);
            ConfigureFeature("dependencies", status);
            ConfigureFeature("directMSI", status);
            ConfigureFeature("portableInstall", status);
        }
    }
}
