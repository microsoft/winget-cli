// -----------------------------------------------------------------------------
// <copyright file="BaseUserSettingsCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Globalization;
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

            // This is based of https://github.com/PowerShell/PowerShell/blob/master/src/Microsoft.PowerShell.Commands.Utility/commands/utility/WebCmdlet/JsonObject.cs.
            // So we can convert JSON to Hashtable for Windows PowerShell and PowerShell Core.
            try
            {
                var obj = JsonConvert.DeserializeObject(
                    content,
                    new JsonSerializerSettings
                    {
                        // This TypeNameHandling setting is required to be secure.
                        TypeNameHandling = TypeNameHandling.None,
                        MetadataPropertyHandling = MetadataPropertyHandling.Ignore,
                        MaxDepth = 1024,
                    });

                // It only makes sense that the deserialized object is a dictionary to start.
                return obj switch
                {
                    JObject dictionary => PopulateHashTableFromJDictionary(dictionary),
                    _ => throw new UserSettingsReadException()
                };
            }
            catch (Exception e)
            {
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

        private static Hashtable PopulateHashTableFromJDictionary(JObject entries)
        {
            Hashtable result = new (entries.Count);
            foreach (var entry in entries)
            {
                switch (entry.Value)
                {
                    case JArray list:
                        {
                            // Array
                            var listResult = PopulateHashTableFromJArray(list);
                            result.Add(entry.Key, listResult);
                            break;
                        }

                    case JObject dic:
                        {
                            // Dictionary
                            var dicResult = PopulateHashTableFromJDictionary(dic);
                            result.Add(entry.Key, dicResult);
                            break;
                        }

                    case JValue value:
                        {
                            result.Add(entry.Key, value.Value);
                            break;
                        }
                }
            }

            return result;
        }

        private static ICollection<object> PopulateHashTableFromJArray(JArray list)
        {
            var result = new object[list.Count];

            for (var index = 0; index < list.Count; index++)
            {
                var element = list[index];

                switch (element)
                {
                    case JArray array:
                        {
                            // Array
                            var listResult = PopulateHashTableFromJArray(array);
                            result[index] = listResult;
                            break;
                        }

                    case JObject dic:
                        {
                            // Dictionary
                            var dicResult = PopulateHashTableFromJDictionary(dic);
                            result[index] = dicResult;
                            break;
                        }

                    case JValue value:
                        {
                            result[index] = value.Value;
                            break;
                        }
                }
            }

            return result;
        }
    }
}
