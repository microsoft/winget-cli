// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Helpers
{
    using System;
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

        /// <summary>
        /// Determine if telemetry can be used. It follows the same telemetry rules as PowerShell.
        /// To opt-out of this telemetry, set the environment variable $env:POWERSHELL_TELEMETRY_OPTOUT to true, yes, or 1.
        /// This method is the same as GetEnvironmentVariableAsBool from PowerShell but only for POWERSHELL_TELEMETRY_OPTOUT.
        /// </summary>
        /// <returns>If telemetry can be used.</returns>
        public static bool CanUseTelemetry()
        {
            var str = Environment.GetEnvironmentVariable("POWERSHELL_TELEMETRY_OPTOUT");
            if (string.IsNullOrEmpty(str))
            {
                return true;
            }

            var boolStr = str.AsSpan();

            if (boolStr.Length == 1)
            {
                if (boolStr[0] == '1')
                {
                    return false;
                }

                if (boolStr[0] == '0')
                {
                    return true;
                }
            }

            if (boolStr.Length == 3 &&
                (boolStr[0] == 'y' || boolStr[0] == 'Y') &&
                (boolStr[1] == 'e' || boolStr[1] == 'E') &&
                (boolStr[2] == 's' || boolStr[2] == 'S'))
            {
                return false;
            }

            if (boolStr.Length == 2 &&
                (boolStr[0] == 'n' || boolStr[0] == 'N') &&
                (boolStr[1] == 'o' || boolStr[1] == 'O'))
            {
                return true;
            }

            if (boolStr.Length == 4 &&
                (boolStr[0] == 't' || boolStr[0] == 'T') &&
                (boolStr[1] == 'r' || boolStr[1] == 'R') &&
                (boolStr[2] == 'u' || boolStr[2] == 'U') &&
                (boolStr[3] == 'e' || boolStr[3] == 'E'))
            {
                return false;
            }

            if (boolStr.Length == 5 &&
                (boolStr[0] == 'f' || boolStr[0] == 'F') &&
                (boolStr[1] == 'a' || boolStr[1] == 'A') &&
                (boolStr[2] == 'l' || boolStr[2] == 'L') &&
                (boolStr[3] == 's' || boolStr[3] == 'S') &&
                (boolStr[4] == 'e' || boolStr[4] == 'E'))
            {
                return true;
            }

            return true;
        }
    }
}
