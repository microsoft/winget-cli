// -----------------------------------------------------------------------------
// <copyright file="PowerShellExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Extensions
{
    using System.Collections.ObjectModel;
    using System.Management.Automation;
    using System.Text;
    using Microsoft.PowerShell.Commands;

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

            var result = pwsh.Invoke(null, settings);

            pwsh.ValidateErrorStream();

            return result;
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

            var result = pwsh.Invoke<T>(null, settings);

            pwsh.ValidateErrorStream();

            return result;
        }

        /// <summary>
        /// Throws <see cref="WriteErrorException"/> if there are streams in the stream error.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        public static void ValidateErrorStream(this PowerShell pwsh)
        {
            if (pwsh.HadErrors)
            {
                var psStreamBuilder = new StringBuilder();
                foreach (var line in pwsh.Streams.Error)
                {
                    psStreamBuilder.AppendLine(line.ToString());
                }

                throw new WriteErrorException(psStreamBuilder.ToString());
            }
        }
    }
}
