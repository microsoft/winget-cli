// -----------------------------------------------------------------------------
// <copyright file="BaseUserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System;
    using System.Collections;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.PowerShell.Commands;
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
        /// Returns the contents of the settings file as Hashtable.
        /// </summary>
        /// <returns>Contents of settings file.</returns>
        protected Hashtable GetLocalSettingsAsHashtable()
        {
            var content = File.Exists(WinGetSettingsFilePath) ?
                File.ReadAllText(WinGetSettingsFilePath) :
                string.Empty;

            return this.ConvertToHashtable(content);
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

        /// <summary>
        /// Uses Powershell ConvertFrom-Json to convert a JSON to a Hashtable.
        /// </summary>
        /// <param name="content">Content.</param>
        /// <returns>Hashtable.</returns>
        protected Hashtable ConvertToHashtable(string content)
        {
            if (string.IsNullOrEmpty(content))
            {
                return new Hashtable();
            }

#if POWERSHELL_WINDOWS
            throw new PSNotImplementedException();
#else
            // Powershell's documentation says that the object being return is either a PSObject
            // or a Hashtable depending on the value of returnHashtable, but is not true. The return
            // type is a PSObject and the BaseObject is either a OrderedHashtable or a PSCustomObject
            // depending on returnHashtable.
            try
            {
                var result = JsonObject.ConvertFromJson(content, returnHashtable: true, out ErrorRecord error) as PSObject;
                if (error is not null)
                {
                    throw new UserSettingsReadException(error.Exception);
                }

                return result.BaseObject as Hashtable;
            }
            catch (Exception e)
            {
                throw new UserSettingsReadException(e);
            }
#endif
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
