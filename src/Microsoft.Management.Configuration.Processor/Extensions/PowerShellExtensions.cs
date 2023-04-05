﻿// -----------------------------------------------------------------------------
// <copyright file="PowerShellExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System.Collections.ObjectModel;
    using System.Management.Automation;
    using System.Text;

    /// <summary>
    /// Extensions methods for <see cref="PowerShell"/> class.
    /// </summary>
    internal static class PowerShellExtensions
    {
        /// <summary>
        /// Calls Invoke with <see cref="ActionPreference.Stop"/> to stop execution of commands.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <returns>Collection of PSObjects representing output.</returns>
        public static Collection<PSObject> InvokeAndStopOnError(this PowerShell pwsh)
        {
            var settings = new PSInvocationSettings()
            {
                ErrorActionPreference = ActionPreference.Stop,
            };

            return pwsh.Invoke(null, settings);
        }

        /// <summary>
        /// Calls Invoke with <see cref="ActionPreference.Stop"/> to stop execution of commands.
        /// </summary>
        /// <typeparam name="T">Type of output object(s) expected from the command invocation.</typeparam>
        /// <param name="pwsh">PowerShell.</param>
        /// <returns>Collection of the type output representing output.</returns>
        public static Collection<T> InvokeAndStopOnError<T>(this PowerShell pwsh)
        {
            var settings = new PSInvocationSettings()
            {
                ErrorActionPreference = ActionPreference.Stop,
            };

            return pwsh.Invoke<T>(null, settings);
        }

        /// <summary>
        /// Gets the error stream message if any.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <returns>Error message. Null if none.</returns>
        public static string? GetErrorMessage(this PowerShell pwsh)
        {
            if (pwsh.HadErrors)
            {
                var psStreamBuilder = new StringBuilder();
                foreach (var line in pwsh.Streams.Error)
                {
                    psStreamBuilder.AppendLine(line.ToString());
                }

                return psStreamBuilder.ToString();
            }

            return null;
        }
    }
}
