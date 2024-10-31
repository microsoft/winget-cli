// -----------------------------------------------------------------------------
// <copyright file="UserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System;
    using System.Collections;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Common.Command;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Class used by the user settings cmdlets.
    /// </summary>
    public sealed class UserSettingsCommand : BaseCommand
    {
        private const string SchemaKey = "$schema";
        private const string SchemaValue = "https://aka.ms/winget-settings.schema.json";

        private static string? winGetSettingsFilePath;

        /// <summary>
        /// Initializes a new instance of the <see cref="UserSettingsCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        public UserSettingsCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
            // Doing it in the static constructor will show the user running in system context:
            // The type initializer for 'Microsoft.WinGet.Client.Engine.Commands.UserSettingsCommand' threw an exception.
            // Here would be "The specified method is not supported."
            if (winGetSettingsFilePath == null)
            {
                var wingetCliWrapper = new WingetCLIWrapper();
                var settingsResult = wingetCliWrapper.RunCommand(this, "settings", "export");

                // Read the user settings file property.
                var userSettingsFile = Utilities.ConvertToHashtable(settingsResult.StdOut)["userSettingsFile"] ?? throw new ArgumentNullException("userSettingsFile");
                winGetSettingsFilePath = (string)userSettingsFile;
            }
        }

        /// <summary>
        /// Get-WinGetUserSetting.
        /// </summary>
        public void Get()
        {
            this.Write(StreamType.Object, this.GetLocalSettingsAsHashtable());
        }

        /// <summary>
        /// Test-WinGetUserSetting.
        /// </summary>
        /// <param name="userSettings">Input user settings.</param>
        /// <param name="ignoreNotSet">Ignore comparing settings that are not part of the input.</param>
        public void Test(Hashtable userSettings, bool ignoreNotSet)
        {
            this.Write(StreamType.Object, this.CompareUserSettings(userSettings, ignoreNotSet));
        }

        /// <summary>
        /// Set-WinGetUserSetting.
        /// </summary>
        /// <param name="userSettings">Input user settings.</param>
        /// <param name="merge">Merge the current user settings and the input settings.</param>
        public void Set(Hashtable userSettings, bool merge)
        {
            var newSettings = HashtableToJObject(userSettings);

            // Merge settings.
            if (merge)
            {
                var currentSettings = this.LocalSettingsFileToJObject();

                // To make the input settings triumph, they need to be merged into the existing settings.
                currentSettings.Merge(newSettings, new JsonMergeSettings
                {
                    MergeArrayHandling = MergeArrayHandling.Union,
                    MergeNullValueHandling = MergeNullValueHandling.Ignore,
                });

                newSettings = currentSettings;
            }

            // Add schema if not there.
            if (!newSettings.ContainsKey(SchemaKey))
            {
                newSettings.Add(SchemaKey, SchemaValue);
            }

            var orderedSettings = this.CreateAlphabeticallyOrderedJObject(newSettings);

            // Write settings.
            var settingsJson = orderedSettings.ToString(Formatting.Indented);
            File.WriteAllText(
                winGetSettingsFilePath!,
                settingsJson);

            this.Write(StreamType.Object, Utilities.ConvertToHashtable(settingsJson));
        }

        private static JObject HashtableToJObject(Hashtable hashtable)
        {
            return (JObject)JToken.FromObject(hashtable);
        }

        private Hashtable GetLocalSettingsAsHashtable()
        {
            var content = File.Exists(winGetSettingsFilePath) ?
                File.ReadAllText(winGetSettingsFilePath) :
                string.Empty;

            return Utilities.ConvertToHashtable(content);
        }

        private JObject LocalSettingsFileToJObject()
        {
            try
            {
                return File.Exists(winGetSettingsFilePath) ?
                    JObject.Parse(File.ReadAllText(winGetSettingsFilePath)) :
                    new JObject();
            }
            catch (JsonReaderException e)
            {
                this.Write(StreamType.Verbose, e.Message);
                throw new UserSettingsReadException(e);
            }
        }

        private bool CompareUserSettings(Hashtable userSettings, bool ignoreNotSet)
        {
            try
            {
                var currentSettings = this.LocalSettingsFileToJObject();
                var newSettings = HashtableToJObject(userSettings);

                // Don't fail because of the schema.
                if (currentSettings.ContainsKey(SchemaKey))
                {
                    currentSettings.Remove(SchemaKey);
                }

                if (newSettings.ContainsKey(SchemaKey))
                {
                    newSettings.Remove(SchemaKey);
                }

                if (ignoreNotSet)
                {
                    return this.PartialDeepEquals(newSettings, currentSettings);
                }

                return JToken.DeepEquals(newSettings, currentSettings);
            }
            catch (Exception e)
            {
                this.Write(StreamType.Verbose, e.Message);
                return false;
            }
        }

        /// <summary>
        /// Partially compares json. All properties and values of json must exist and have the same value
        /// as otherJson.
        /// This doesn't support deep JArray object comparison, but we don't have arrays of type object so far :).
        /// </summary>
        /// <param name="json">Main json.</param>
        /// <param name="otherJson">otherJson.</param>
        /// <returns>True is otherJson partially contains json.</returns>
        private bool PartialDeepEquals(JToken json, JToken? otherJson)
        {
            if (JToken.DeepEquals(json, otherJson))
            {
                return true;
            }

            if (otherJson == null)
            {
                return false;
            }

            // If they are a JValue (string, integer, date, etc) or they are a JArray and DeepEquals fails then not equal.
            if ((json is JValue && otherJson is JValue) ||
                (json is JArray && otherJson is JArray))
            {
                this.Write(
                    StreamType.Verbose,
                    $"'{json.ToString(Formatting.None)}' != '{otherJson.ToString(Formatting.None)}'");
                return false;
            }

            // If its not the same type then don't bother.
            if (json.Type != otherJson.Type)
            {
                this.Write(
                    StreamType.Verbose,
                    $"Mismatch types '{json.ToString(Formatting.None)}' '{otherJson.ToString(Formatting.None)}'");
                return false;
            }

            // Look deeply.
            if (json.Type == JTokenType.Object)
            {
                var jObject = (JObject)json;
                var otherJObject = (JObject)otherJson;

                var properties = jObject.Properties();
                foreach (var property in properties)
                {
                    // If the property is not there then give up.
                    if (!otherJObject.ContainsKey(property.Name))
                    {
                        this.Write(StreamType.Verbose, $"{property.Name} not found.");
                        return false;
                    }

                    if (!this.PartialDeepEquals(property.Value, otherJObject.GetValue(property.Name)))
                    {
                        // Found inequality within a property. We are done.
                        return false;
                    }
                }
            }

            return true;
        }

        /// <summary>
        /// Helper method to order alphabetically properties. Newtonsoft doesn't have a nice way
        /// to do it via a custom JsonConverter.
        /// </summary>
        /// <param name="jObject">JObject.</param>
        /// <returns>New ordered JObject.</returns>
        private JObject CreateAlphabeticallyOrderedJObject(JObject jObject)
        {
            JObject newJObject = new ();
            var orderedProperties = jObject.Properties().OrderBy(p => p.Name, StringComparer.Ordinal);
            foreach (var property in orderedProperties)
            {
                if (property.Value.Type == JTokenType.Object)
                {
                    newJObject.Add(
                        property.Name,
                        this.CreateAlphabeticallyOrderedJObject((JObject)property.Value));
                }
                else
                {
                    newJObject.Add(property);
                }
            }

            return newJObject;
        }
    }
}
