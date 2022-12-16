// -----------------------------------------------------------------------------
// <copyright file="BaseUserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Common
{
    using System.Collections;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Helpers;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Base command for user settings cmdlets.
    /// </summary>
    public abstract class BaseUserSettingsCommand : PSCmdlet
    {
        /// <summary>
        /// The schema key.
        /// </summary>
        protected const string SchemaKey = "$schema";

        /// <summary>
        /// The default value of the schema property.
        /// </summary>
        protected const string SchemaValue = "https://aka.ms/winget-settings.schema.json";

        /// <summary>
        /// Gets the path for the winget settings.
        /// </summary>
        protected static string WinGetSettingsFilePath
        {
            get
            {
                return GetUserSettingsPath();
            }
        }

        /// <summary>
        /// Converts a Hashtable to a JObject object.
        /// </summary>
        /// <param name="hashtable">Hashtable.</param>
        /// <returns>JObject.</returns>
        protected static JObject HashtableToJObject(Hashtable hashtable)
        {
            return (JObject)JToken.FromObject(hashtable);
        }

        /// <summary>
        /// Returns the content of the settings file.
        /// </summary>
        /// <returns>Contest of  settings file.</returns>
        protected static string GetLocalSettingsFileContents()
        {
            if (!File.Exists(WinGetSettingsFilePath))
            {
                return string.Empty;
            }

            // The original plan was to return the contents deserialized as a Hashtable, but that
            // will make only the top level keys be treated as Hashtable keys and their value would
            // be a JObjects. This make them really awkward to handle. If we really want to we will
            // need to implement a custom deserializer or manually walk the json and convert all
            // keys to hash tables. For now a caller can just pipe this to ConvertTo-Json.
            return File.ReadAllText(WinGetSettingsFilePath);
        }

        /// <summary>
        /// Converts the current local settings file into a JObject object.
        /// </summary>
        /// <returns>User settings as JObject.</returns>
        protected static JObject LocalSettingsFileToJObject()
        {
            return JObject.Parse(GetLocalSettingsFileContents());
        }

        private static string GetUserSettingsPath()
        {
            var wingetCliWrapper = new WingetCLIWrapper();
            var settingsResult = wingetCliWrapper.RunCommand("settings", "export");

            // Read the user settings file property.
            var serialized = JObject.Parse(settingsResult.StdOut);
            return (string)serialized.GetValue("userSettingsFile");
        }
    }
}
