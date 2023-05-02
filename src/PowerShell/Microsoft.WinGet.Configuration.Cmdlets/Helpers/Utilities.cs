// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Helpers
{
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.PowerShell;

    /// <summary>
    /// Utilities for this cmdlets.
    /// </summary>
    internal static class Utilities
    {
        /// <summary>
        /// Gets the execution policy.
        /// </summary>
        /// <returns>ExecutionPolicy.</returns>
        public static ExecutionPolicy GetExecutionPolicy()
        {
            var ps = PowerShell.Create(RunspaceMode.CurrentRunspace);
            return ps.AddCommand("Get-ExecutionPolicy").Invoke<ExecutionPolicy>().First();
        }
    }
}
