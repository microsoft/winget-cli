﻿// -----------------------------------------------------------------------------
// <copyright file="Utilities.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Helpers
{
    using System;
    using System.Management.Automation;
    using System.Management.Automation.Host;

    /// <summary>
    /// Helper methods.
    /// </summary>
    internal static class Utilities
    {
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
    }
}
