// -----------------------------------------------------------------------------
// <copyright file="BaseUserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System.Collections;
    using System.IO;
    using Microsoft.WinGet.Client.Exceptions;
    using Microsoft.WinGet.Client.Helpers;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Base command for user settings cmdlets.
    /// </summary>
    public abstract class BaseUserSettingsCommand : BaseCommand
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
        /// Returns the contents of the settings file.
        /// The original plan was to return the contents deserialized as a Hashtable, but that
        /// will make only the top level keys be treated as Hashtable keys and their value would
        /// be a JObject. This make them really awkward to handle. If we really want to we will
        /// need to implement a custom deserializer or manually walk the json and convert all
        /// keys to hash tables. For now a caller can just pipe this to ConvertTo-Json.
        /// </summary>
        /// <returns>Contents of settings file.</returns>
        protected string GetLocalSettingsFileContents()
        {
            // Validate is at least json.
            _ = this.LocalSettingsFileToJObject();

            return File.Exists(WinGetSettingsFilePath) ?
                File.ReadAllText(WinGetSettingsFilePath) :
                string.Empty;
        }

        /// <summary>
        /// Converts the current local settings file into a JObject object.
        /// </summary>
        /// <returns>User settings as JObject.</returns>
        protected JObject LocalSettingsFileToJObject()
        {
            try
            {
                return File.Exists(WinGetSettingsFilePath) ?
                    JObject.Parse(File.ReadAllText(WinGetSettingsFilePath)) :
                    new JObject();
            }
            catch (JsonReaderException e)
            {
                this.WriteDebug(e.Message);
                throw new UserSettingsReadException(e);
            }
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
