// -----------------------------------------------------------------------------
// <copyright file="WinGetSettingsHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
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
                        { "openLogsArgument", false },
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
    }
}
