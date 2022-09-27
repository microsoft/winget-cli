// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
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
            TestCommon.TearDownTestSource();
        }

        // TODO: If/when cert pinning is implemented on the packaged index source, useGroupPolicyForTestSource should be set to default true
        //       to enable testing it by default.  Until then, leaving this here...
        public void ResetTestSource(bool useGroupPolicyForTestSource = false)
        {
            TestCommon.SetupTestSource(useGroupPolicyForTestSource);
        }

        public void ConfigureFeature(string featureName, bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(Constants.LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath)));
            JObject experimentalFeatures = (JObject)settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;

            File.WriteAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath), settingsJson.ToString());
        }

        public void ConfigureInstallBehavior(string settingName, string value)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(Constants.LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath)));
            JObject installBehavior = (JObject)settingsJson["installBehavior"];
            installBehavior[settingName] = value;

            File.WriteAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath), settingsJson.ToString());
        }

        public void InitializeAllFeatures(bool status)
        {
            ConfigureFeature("experimentalArg", status);
            ConfigureFeature("experimentalCmd", status);
            ConfigureFeature("dependencies", status);
            ConfigureFeature("directMSI", status);
            ConfigureFeature("zipInstall", status);
            ConfigureFeature("openLogsArgument", status);
        }
    }
}
