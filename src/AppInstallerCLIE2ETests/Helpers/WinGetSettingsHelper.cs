// -----------------------------------------------------------------------------
// <copyright file="WinGetSettingsHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Helpers
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
        /// Gets or sets the experimental features that should be forcibly enabled.
        /// </summary>
        public static string[] ForcedExperimentalFeatures { get; set; }

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
            Hashtable experimentalFeatures = new Hashtable();

            var forcedExperimentalFeatures = ForcedExperimentalFeatures;
            if (forcedExperimentalFeatures != null)
            {
                foreach (var feature in forcedExperimentalFeatures)
                {
                    experimentalFeatures[feature] = true;
                }
            }

            var settingsJson = new Hashtable()
            {
                {
                    "$schema",
                    "https://aka.ms/winget-settings.schema.json"
                },
                {
                    "experimentalFeatures",
                    experimentalFeatures
                },
                {
                    "debugging",
                    new Hashtable()
                    {
                        { "enableSelfInitiatedMinidump", true },
                        { "keepAllLogFiles", true },
                    }
                },
                {
                    "installBehavior",
                    new Hashtable()
                    {
                    }
                },
                {
                    "configureBehavior",
                    new Hashtable()
                    {
                    }
                },
            };

            // Run winget one time to initialize settings directory
            // when running in unpackaged context
            TestCommon.RunAICLICommand(string.Empty, "-v");

            SetWingetSettings(JsonConvert.SerializeObject(settingsJson, Formatting.Indented));
        }

        /// <summary>
        /// Configure experimental features.
        /// </summary>
        /// <param name="featureName">Feature name.</param>
        /// <param name="status">Status.</param>
        public static void ConfigureFeature(string featureName, bool status)
        {
            JObject settingsJson = GetJsonSettingsObject("experimentalFeatures");
            ConfigureFeature(settingsJson, featureName, status);
            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Configure the install behavior.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public static void ConfigureInstallBehavior(string settingName, string value)
        {
            JObject settingsJson = GetJsonSettingsObject("installBehavior");
            var installBehavior = settingsJson["installBehavior"];
            installBehavior[settingName] = value;

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Configure the configuration behavior.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public static void ConfigureConfigureBehavior(string settingName, string value)
        {
            JObject settingsJson = GetJsonSettingsObject("configureBehavior");
            var configureBehavior = settingsJson["configureBehavior"];
            configureBehavior[settingName] = value;

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Configure the install behavior preferences.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public static void ConfigureInstallBehaviorPreferences(string settingName, string value)
        {
            JObject settingsJson = GetJsonSettingsObject("installBehavior");
            var installBehavior = settingsJson["installBehavior"];

            if (installBehavior["preferences"] == null)
            {
                installBehavior["preferences"] = new JObject();
            }

            var preferences = installBehavior["preferences"];
            preferences[settingName] = value;

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Configure the install behavior preferences.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value array.</param>
        public static void ConfigureInstallBehaviorPreferences(string settingName, string[] value)
        {
            JObject settingsJson = GetJsonSettingsObject("installBehavior");
            var installBehavior = settingsJson["installBehavior"];

            if (installBehavior["preferences"] == null)
            {
                installBehavior["preferences"] = new JObject();
            }

            var preferences = installBehavior["preferences"];
            preferences[settingName] = new JArray(value);

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Configure the install behavior requirements.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value.</param>
        public static void ConfigureInstallBehaviorRequirements(string settingName, string value)
        {
            JObject settingsJson = GetJsonSettingsObject("installBehavior");
            var installBehavior = settingsJson["installBehavior"];

            if (installBehavior["requirements"] == null)
            {
                installBehavior["requirements"] = new JObject();
            }

            var requirements = installBehavior["requirements"];
            requirements[settingName] = value;

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Configure the install behavior requirements.
        /// </summary>
        /// <param name="settingName">Setting name.</param>
        /// <param name="value">Setting value array.</param>
        public static void ConfigureInstallBehaviorRequirements(string settingName, string[] value)
        {
            JObject settingsJson = GetJsonSettingsObject("installBehavior");
            var installBehavior = settingsJson["installBehavior"];

            if (installBehavior["requirements"] == null)
            {
                installBehavior["requirements"] = new JObject();
            }

            var requirements = installBehavior["requirements"];
            requirements[settingName] = new JArray(value);

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Initialize all features.
        /// </summary>
        /// <param name="status">Initialized feature value.</param>
        public static void InitializeAllFeatures(bool status)
        {
            JObject settingsJson = GetJsonSettingsObject("experimentalFeatures");

            ConfigureFeature(settingsJson, "experimentalArg", status);
            ConfigureFeature(settingsJson, "experimentalCmd", status);
            ConfigureFeature(settingsJson, "directMSI", status);
            ConfigureFeature(settingsJson, "windowsFeature", status);
            ConfigureFeature(settingsJson, "resume", status);
            ConfigureFeature(settingsJson, "reboot", status);

            SetWingetSettings(settingsJson);
        }

        /// <summary>
        /// Configure experimental features.
        /// </summary>
        /// <param name="settingsJson">The settings JSON object to modify.</param>
        /// <param name="featureName">Feature name.</param>
        /// <param name="status">Status.</param>
        private static void ConfigureFeature(JObject settingsJson, string featureName, bool status)
        {
            var experimentalFeatures = settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;
        }

        private static JObject GetJsonSettingsObject(string objectName)
        {
            JObject settingsJson = JObject.Parse(File.ReadAllText(TestSetup.Parameters.SettingsJsonFilePath));

            if (!settingsJson.ContainsKey(objectName))
            {
                settingsJson[objectName] = new JObject();
            }

            return settingsJson;
        }

        /// <summary>
        /// Converts a JObject to a string and writes to the settings file.
        /// </summary>
        /// <param name="settingsJson">Settings to set.</param>
        private static void SetWingetSettings(JObject settingsJson)
        {
            var forcedExperimentalFeatures = ForcedExperimentalFeatures;
            if (forcedExperimentalFeatures != null)
            {
                foreach (var feature in forcedExperimentalFeatures)
                {
                    ConfigureFeature(settingsJson, feature, true);
                }
            }

            SetWingetSettings(settingsJson.ToString());
        }

        /// <summary>
        /// Writes string to settings file.
        /// </summary>
        /// <param name="settings">Settings as string.</param>
        private static void SetWingetSettings(string settings)
        {
            File.WriteAllText(TestSetup.Parameters.SettingsJsonFilePath, settings);
        }
    }
}
