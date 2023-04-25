// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Helpers
{
    using System;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;

    /// <summary>
    /// Utilities for this cmdlets.
    /// </summary>
    internal static class Utilities
    {
        /// <summary>
        /// Change the current session to the location where the cmdlet
        /// got executed.
        /// </summary>
        public static void ChangeToCurrentSessionLocation()
        {
            var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);
            var results = ps.AddCommand("Get-Location").Invoke<PathInfo>();
            if (results is null || results.Count == 0)
            {
                throw new InvalidOperationException();
            }

            Directory.SetCurrentDirectory(results.First().Path);
        }
    }
}
