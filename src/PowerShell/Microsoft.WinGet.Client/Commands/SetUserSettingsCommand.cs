// -----------------------------------------------------------------------------
// <copyright file="SetUserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Collections;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Sets the specified user settings into the winget user settings. If the merge switch is on, merges current user
    /// settings with the input settings. Otherwise, overwrites the input settings.
    /// </summary>
    [Cmdlet(VerbsCommon.Set, Constants.WinGetNouns.UserSettings)]
    [OutputType(typeof(Hashtable))]
    public sealed class SetUserSettingsCommand : BaseUserSettingsCommand
    {
        /// <summary>
        /// Gets or sets the input user settings.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipelineByPropertyName = true)]
        public Hashtable UserSettings { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to merge the current user settings and the input settings.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Merge { get; set; }

        /// <summary>
        /// Process input of cmdlet.
        /// </summary>
        protected override void ProcessRecord()
        {
            var newSettings = HashtableToJObject(this.UserSettings);

            // Merge settings.
            if (this.Merge.ToBool())
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

            var orderedSettings = CreateAlphabeticallyOrderedJObject(newSettings);

            // Write settings.
            var settingsJson = orderedSettings.ToString(Formatting.Indented);
            File.WriteAllText(
                WinGetSettingsFilePath,
                settingsJson);

            this.WriteObject(this.ConvertToHashtable(settingsJson));
        }

        /// <summary>
        /// Helper method to order alphabetically properties. Newtonsoft doesn't have a nice way
        /// to do it via a custom JsonConverter.
        /// </summary>
        /// <param name="jObject">JObject.</param>
        /// <returns>New ordered JObject.</returns>
        private static JObject CreateAlphabeticallyOrderedJObject(JObject jObject)
        {
            JObject newJObject = new ();
            var orderedProperties = jObject.Properties().OrderBy(p => p.Name, StringComparer.Ordinal);
            foreach (var property in orderedProperties)
            {
                if (property.Value.Type == JTokenType.Object)
                {
                    newJObject.Add(
                        property.Name,
                        CreateAlphabeticallyOrderedJObject((JObject)property.Value));
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
