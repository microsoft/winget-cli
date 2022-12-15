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
            var currentSettings = LocalSettingsFileToJObject();
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
                return this.PartialCompare(newSettings, currentSettings);
            }

            return JToken.DeepEquals(newSettings, currentSettings);
        }

        private bool PartialCompare(JObject jObject, JObject other)
        {
            try
            {
                this.PartialDeepEquals(jObject, other);
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        // This doesn't support JArray object comparison, but we don't have arrays of type object so far.
        private void PartialDeepEquals(JToken jToken, JToken other)
        {
            if (jToken.Type != other.Type)
            {
                string error = $"Mismatch types '{jToken.ToString(Newtonsoft.Json.Formatting.None)}' " +
                    $"'{other.ToString(Newtonsoft.Json.Formatting.None)}'";
                this.WriteVerbose(error);
                throw new Exception(error);
            }

            if (!JToken.DeepEquals(jToken, other))
            {
                if (jToken.Type == JTokenType.Object)
                {
                    var jObject = (JObject)jToken;
                    var otherJObject = (JObject)other;

                    var properties = jObject.Properties();
                    foreach (var property in properties)
                    {
                        // If the property is not there then give up.
                        if (!otherJObject.ContainsKey(property.Name))
                        {
                            string error = $"{property.Name} not found.";
                            this.WriteVerbose(error);
                            throw new Exception(error);
                        }

                        this.PartialDeepEquals(
                                property.Value,
                                otherJObject.GetValue(property.Name));
                    }
                }
                else if (jToken is JValue)
                {
                    // If this is a JValue (string, integer, date, etc) and DeepEquals fails then is not equal.
                    string error = $"'{jToken.ToString(Newtonsoft.Json.Formatting.None)}' != " +
                        $"'{other.ToString(Newtonsoft.Json.Formatting.None)}'";
                    this.WriteVerbose(error);
                    throw new Exception(error);
                }
            }
        }
    }
}
