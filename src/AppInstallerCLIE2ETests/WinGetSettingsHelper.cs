// -----------------------------------------------------------------------------
// <copyright file="WinGetSettingsHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.Collections;
    using System.IO;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Helper class to set winget settings.
    /// </summary>
    internal static class WinGetSettingsHelper
    {
        /// <summary>
        /// Gets the user settings path by calling winget settings export.
        /// </summary>
        /// <returns>Expanded path for user settings.</returns>
        public static string GetUserSettingsPath()
        {
            var result = TestCommon.RunAICLICommand("settings", "export");
            var output = result.StdOut;
            var serialized = JObject.Parse(output);
            return (string)serialized.GetValue("userSettingsFile");
        }

        /// <summary>
        /// Initialize settings.
        /// </summary>
        public static void InitializeWingetSettings()
        {
            var settingsJson = new Hashtable()
            {
                {
                    "experimentalFeatures",
                    new Hashtable()
                    {
                        { "experimentalArg", false },
                        { "experimentalCmd", false },
                        { "dependencies", false },
                        { "directMSI", false },
                    }
                },
                {
                    "debugging",
                    new Hashtable()
                    {
                        { "enableSelfInitiatedMinidump", false },
                    }
                },
                {
                    "installBehavior",
                    new Hashtable()
                    {
                    }
                },
            };

            // Run winget one time to initialize settings directory
            // when running in unpackaged context
            TestCommon.RunAICLICommand(string.Empty, "-v");

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Converts a hashtable to json and writes to the settings file.
        /// </summary>
        /// <param name="settingsJson">Settings to set.</param>
        public static void SetWingetSettings(Hashtable settingsJson)
        {
            SetWingetSettings(JsonConvert.SerializeObject(settingsJson, Formatting.Indented));
        }

        /// <summary>
        /// Writes string to settings file.
        /// </summary>
        /// <param name="settings">Settings as string.</param>
        public static void SetWingetSettings(string settings)
        {
            File.WriteAllText(TestCommon.SettingsJsonFilePath, settings);
        }

        /// <summary>
        /// Configure experimental features.
        /// </summary>
        /// <param name="featureName">Feature name.</param>
        /// <param name="status">Status.</param>
        public static void ConfigureFeature(string featureName, bool status)
        {
            JObject settingsJson = JObject.Parse(File.ReadAllText(TestCommon.SettingsJsonFilePath));

            if (!settingsJson.ContainsKey("experimentalFeatures"))
            {
                settingsJson["experimentalFeatures"] = new JObject();
            }

            var experimentalFeatures = settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;

            File.WriteAllText(TestCommon.SettingsJsonFilePath, settingsJson.ToString());
        }

        /// <summary>
        /// Configure the install behavior.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public static void ConfigureInstallBehavior(string settingName, string value)
        {
            JObject settingsJson = JObject.Parse(File.ReadAllText(TestCommon.SettingsJsonFilePath));

            if (!settingsJson.ContainsKey("installBehavior"))
            {
                settingsJson["installBehavior"] = new JObject();
            }

            var installBehavior = settingsJson["installBehavior"];
            installBehavior[settingName] = value;

            File.WriteAllText(TestCommon.SettingsJsonFilePath, settingsJson.ToString());
        }

        /// <summary>
        /// Configure the install behavior preferences.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public static void ConfigureInstallBehaviorPreferences(string settingName, string value)
        {
            JObject settingsJson = JObject.Parse(File.ReadAllText(TestCommon.SettingsJsonFilePath));

            if (!settingsJson.ContainsKey("installBehavior"))
            {
                settingsJson["installBehavior"] = new JObject();
            }

            var installBehavior = settingsJson["installBehavior"];

            if (installBehavior["preferences"] == null)
            {
                installBehavior["preferences"] = new JObject();
            }

            var preferences = installBehavior["preferences"];
            preferences[settingName] = value;

            File.WriteAllText(TestCommon.SettingsJsonFilePath, settingsJson.ToString());
        }

        /// <summary>
        /// Configure the install behavior requirements.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public static void ConfigureInstallBehaviorRequirements(string settingName, string value)
        {
            JObject settingsJson = JObject.Parse(File.ReadAllText(TestCommon.SettingsJsonFilePath));

            if (!settingsJson.ContainsKey("installBehavior"))
            {
                settingsJson["installBehavior"] = new JObject();
            }

            var installBehavior = settingsJson["installBehavior"];

            if (installBehavior["requirements"] == null)
            {
                installBehavior["requirements"] = new JObject();
            }

            var requirements = installBehavior["requirements"];
            requirements[settingName] = value;

            File.WriteAllText(TestCommon.SettingsJsonFilePath, settingsJson.ToString());
        }

        /// <summary>
        /// Initialize all features.
        /// </summary>
        /// <param name="status">Initialized feature value.</param>
        public static void InitializeAllFeatures(bool status)
        {
            ConfigureFeature("experimentalArg", status);
            ConfigureFeature("experimentalCmd", status);
            ConfigureFeature("dependencies", status);
            ConfigureFeature("directMSI", status);
            ConfigureFeature("pinning", status);
            ConfigureFeature("configuration", status);
            ConfigureFeature("windowsFeature", status);
        }
    }
}
