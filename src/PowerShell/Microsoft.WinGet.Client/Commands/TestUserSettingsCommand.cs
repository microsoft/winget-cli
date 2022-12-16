// -----------------------------------------------------------------------------
// <copyright file="TestUserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Collections;
    using System.Management.Automation;
    using System.Management.Automation.Language;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// Compare the specified user settings with the winget user settings.
    /// </summary>
    [Cmdlet(VerbsDiagnostic.Test, Constants.WinGetNouns.UserSettings)]
    [OutputType(typeof(bool))]
    public sealed class TestUserSettingsCommand : BaseUserSettingsCommand
    {
        /// <summary>
        /// Gets or sets the input user settings.
        /// </summary>
        [Parameter(
            Mandatory = true,
            ValueFromPipelineByPropertyName = true)]
        public Hashtable UserSettings { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to ignore comparing settings that are not part of the input.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public SwitchParameter IgnoreNotSet { get; set; }

        /// <summary>
        /// Process the cmdlet and writes the result of the comparison.
        /// </summary>
        protected override void ProcessRecord()
        {
            this.WriteObject(this.CompareUserSettings());
        }

        private bool CompareUserSettings()
        {
            try
            {
                var currentSettings = this.LocalSettingsFileToJObject();
                var newSettings = HashtableToJObject(this.UserSettings);

                // Don't fail because of the schema.
                if (currentSettings.ContainsKey(SchemaKey))
                {
                    currentSettings.Remove(SchemaKey);
                }

                if (newSettings.ContainsKey(SchemaKey))
                {
                    newSettings.Remove(SchemaKey);
                }

                if (this.IgnoreNotSet.ToBool())
                {
                    return this.PartialDeepEquals(newSettings, currentSettings);
                }

                return JToken.DeepEquals(newSettings, currentSettings);
            }
            catch (Exception e)
            {
                this.WriteDebug(e.Message);
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
        private bool PartialDeepEquals(JToken json, JToken otherJson)
        {
            if (JToken.DeepEquals(json, otherJson))
            {
                return true;
            }

            // If they are a JValue (string, integer, date, etc) or they are a JArray and DeepEquals fails then not equal.
            if ((json is JValue && otherJson is JValue) ||
                (json is JArray && otherJson is JArray))
            {
                this.WriteDebug($"'{json.ToString(Newtonsoft.Json.Formatting.None)}' != " +
                    $"'{otherJson.ToString(Newtonsoft.Json.Formatting.None)}'");
                return false;
            }

            // If its not the same type then don't bother.
            if (json.Type != otherJson.Type)
            {
                this.WriteDebug($"Mismatch types '{json.ToString(Newtonsoft.Json.Formatting.None)}' " +
                    $"'{otherJson.ToString(Newtonsoft.Json.Formatting.None)}'");
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
                        this.WriteDebug($"{property.Name} not found.");
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
    }
}
