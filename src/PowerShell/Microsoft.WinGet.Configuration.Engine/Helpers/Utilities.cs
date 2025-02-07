// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using System;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Host;
    using System.Security.Principal;
    using Microsoft.Management.Configuration;
    using Microsoft.WinGet.Configuration.Engine.PSObjects;

    /// <summary>
    /// Helper methods.
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
        /// Helper for StreamType.Information. Creates a HostInformationMessage with
        /// the specified information.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="noNewLine">Add not to add a new line.</param>
        /// <param name="foregroundColor">Optional foreground color.</param>
        /// <param name="backgroundColor">Optional background color.</param>
        /// <returns>The information message.</returns>
        public static HostInformationMessage CreateInformationMessage(
            string message,
            bool noNewLine = false,
            ConsoleColor? foregroundColor = null,
            ConsoleColor? backgroundColor = null)
        {
            var infoMessage = new HostInformationMessage
            {
                Message = message,
                NoNewLine = noNewLine,
            };

            try
            {
                infoMessage.ForegroundColor = foregroundColor;
                infoMessage.BackgroundColor = backgroundColor;
            }
            catch (HostException)
            {
                // Expected if the host is not interactive, or doesn't have Foreground / Background colors.
            }

            return infoMessage;
        }

        /// <summary>
        /// Splits the message into lines.
        /// </summary>
        /// <param name="message">Message.</param>
        /// <param name="maxLines">Max lines.</param>
        /// <returns>Lines.</returns>
        public static string[] SplitIntoLines(string message, int maxLines)
        {
            var lines = message.Split(new char[] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
            if (lines.Length > maxLines)
            {
                return lines.Take(maxLines).ToArray();
            }

            return lines;
        }

        /// <summary>
        /// Converts ConfigurationTestResult value to PSConfigurationTestResult.
        /// </summary>
        /// <param name="value">ConfigurationTestResult value.</param>
        /// <returns>PSConfigurationTestResult.</returns>
        public static PSConfigurationTestResult ToPSConfigurationTestResult(ConfigurationTestResult value)
        {
            return value switch
            {
                ConfigurationTestResult.Unknown => PSConfigurationTestResult.Unknown,
                ConfigurationTestResult.Positive => PSConfigurationTestResult.Positive,
                ConfigurationTestResult.Negative => PSConfigurationTestResult.Negative,
                ConfigurationTestResult.Failed => PSConfigurationTestResult.Failed,
                ConfigurationTestResult.NotRun => PSConfigurationTestResult.NotRun,
                _ => throw new InvalidOperationException(),
            };
        }

        /// <summary>
        /// Converts ConfigurationUnitState value to PSConfigurationUnitState.
        /// </summary>
        /// <param name="value">ConfigurationUnitState value.</param>
        /// <returns>PSConfigurationUnitState.</returns>
        public static PSConfigurationUnitState ToPSConfigurationUnitState(ConfigurationUnitState value)
        {
            return value switch
            {
                ConfigurationUnitState.Unknown => PSConfigurationUnitState.Unknown,
                ConfigurationUnitState.Pending => PSConfigurationUnitState.Pending,
                ConfigurationUnitState.InProgress => PSConfigurationUnitState.InProgress,
                ConfigurationUnitState.Completed => PSConfigurationUnitState.Completed,
                ConfigurationUnitState.Skipped => PSConfigurationUnitState.Skipped,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
