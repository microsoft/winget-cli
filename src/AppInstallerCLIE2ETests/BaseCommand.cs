// -----------------------------------------------------------------------------
// <copyright file="BaseCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using Newtonsoft.Json.Linq;
    using NUnit.Framework;

    /// <summary>
    /// Base command.
    /// </summary>
    public class BaseCommand
    {
        /// <summary>
        /// Set up.
        /// </summary>
        [OneTimeSetUp]
        public void BaseSetup()
        {
            this.ResetTestSource();
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void BaseTeardown()
        {
            TestCommon.TearDownTestSource();
        }

        /// <summary>
        /// Reset test source.
        /// </summary>
        /// <param name="useGroupPolicyForTestSource">Use group policy from test source.</param>
        public void ResetTestSource(bool useGroupPolicyForTestSource = false)
        {
            // TODO: If/when cert pinning is implemented on the packaged index source, useGroupPolicyForTestSource should be set to default true
            //       to enable testing it by default.  Until then, leaving this here...
            TestCommon.SetupTestSource(useGroupPolicyForTestSource);
        }

        /// <summary>
        /// Configure experimental features.
        /// </summary>
        /// <param name="featureName">Feature name.</param>
        /// <param name="status">Status.</param>
        public void ConfigureFeature(string featureName, bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(Constants.LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath)));
            JObject experimentalFeatures = (JObject)settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;

            File.WriteAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath), settingsJson.ToString());
        }

        /// <summary>
        /// Configure the install behavior.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public void ConfigureInstallBehavior(string settingName, string value)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(Constants.LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath)));
            JObject installBehavior = (JObject)settingsJson["installBehavior"];
            installBehavior[settingName] = value;

            File.WriteAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath), settingsJson.ToString());
        }

        /// <summary>
        /// Initialize all features.
        /// </summary>
        /// <param name="status">Initialized feature value.</param>
        public void InitializeAllFeatures(bool status)
        {
            this.ConfigureFeature("experimentalArg", status);
            this.ConfigureFeature("experimentalCmd", status);
            this.ConfigureFeature("dependencies", status);
            this.ConfigureFeature("directMSI", status);
            this.ConfigureFeature("zipInstall", status);
            this.ConfigureFeature("openLogsArgument", status);
        }
    }
}
