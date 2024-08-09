// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Common
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Management.Automation;
    using System.Security.Principal;
    using System.Threading;
    using Microsoft.WinGet.Resources;
    using Newtonsoft.Json;
    using Newtonsoft.Json.Linq;

    /// <summary>
    /// This class contains various helper methods for this project.
    /// </summary>
    internal static class Utilities
    {
        /// <summary>
        /// Gets a value indicating whether the current assembly is executing in an administrative context.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "Windows only API")]
        public static bool ExecutingAsAdministrator
        {
            get
            {
                WindowsIdentity identity = WindowsIdentity.GetCurrent();
                WindowsPrincipal principal = new (identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current assembly is executing as a SYSTEM user.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "Windows only API")]
        public static bool ExecutingAsSystem
        {
            get
            {
                return WindowsIdentity.GetCurrent().IsSystem;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current execution context will use in-proc winget.
        /// </summary>
        public static bool UsesInProcWinget
        {
            get
            {
                return ExecutingAsSystem;
            }
        }

        /// <summary>
        /// Gets a value indicating whether the current thread is executing as STA.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("Interoperability", "CA1416:Validate platform compatibility", Justification = "Windows only API")]
        public static bool ThreadIsSTA
        {
            get
            {
                return Thread.CurrentThread.GetApartmentState() == ApartmentState.STA;
            }
        }

        /// <summary>
        /// Gets the windows app path for local app data.
        /// </summary>
        public static string LocalDataWindowsAppPath
        {
            get
            {
                return Environment.ExpandEnvironmentVariables(@"%LOCALAPPDATA%\Microsoft\WindowsApps");
            }
        }

        /// <summary>
        /// Gets the windows app path for program files.
        /// </summary>
        public static string ProgramFilesWindowsAppPath
        {
            get
            {
                return Environment.ExpandEnvironmentVariables(@"%PROGRAMFILES%\WindowsApps");
            }
        }

        /// <summary>
        /// Throws if not running as admin.
        /// </summary>
        public static void VerifyAdmin()
        {
            if (!Utilities.ExecutingAsAdministrator)
            {
                throw new PSNotSupportedException(Resources.RequiresAdminMessage);
            }
        }

        /// <summary>
        /// Adds the WindowsApp local app data path to the user environment path.
        /// </summary>
        public static void AddWindowsAppToPath()
        {
            var scope = EnvironmentVariableTarget.User;
            string? envPathValue = Environment.GetEnvironmentVariable(Constants.PathEnvVar, scope);
            if (string.IsNullOrEmpty(envPathValue) || !envPathValue.Contains(Utilities.LocalDataWindowsAppPath))
            {
                Environment.SetEnvironmentVariable(
                    Constants.PathEnvVar,
                    $"{envPathValue};{Utilities.LocalDataWindowsAppPath}",
                    scope);
            }
        }

        /// <summary>
        /// This is based of https://github.com/PowerShell/PowerShell/blob/master/src/Microsoft.PowerShell.Commands.Utility/commands/utility/WebCmdlet/JsonObject.cs.
        /// So we can convert JSON to Hashtable for Windows PowerShell and PowerShell Core.
        /// </summary>
        /// <param name="content">String content.</param>
        /// <returns>The hashtable.</returns>
        public static Hashtable ConvertToHashtable(string content)
        {
            if (string.IsNullOrEmpty(content))
            {
                return new Hashtable();
            }

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
                _ => throw new InvalidDataException()
            };
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

        private static ICollection<object?> PopulateHashTableFromJArray(JArray list)
        {
            var result = new object?[list.Count];

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
