// -----------------------------------------------------------------------------
// <copyright file="PSConfigurationProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Configuration;
    using Microsoft.PowerShell.Commands;
    using Microsoft.WinGet.Configuration.Engine.Commands;
    using Microsoft.WinGet.Configuration.Engine.Exceptions;
    using static Microsoft.WinGet.Configuration.Engine.Commands.AsyncCommand;

    /// <summary>
    /// Creates configuration processor and set up diagnostic logging.
    /// If this object is the input of another cmdlet and the cmdlet is not a
    /// long running task (aka not Continue-*) for now the caller is responsible
    /// of updating the AsyncCommand of this object.
    /// In the future we can implement a singleton that handles all the signaling
    /// for main thread actions.
    /// </summary>
    public class PSConfigurationProcessor
    {
        private static readonly object CmdletLock = new ();

        private AsyncCommand diagnosticCommand;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSConfigurationProcessor"/> class.
        /// </summary>
        /// <param name="factory">Factory.</param>
        /// <param name="diagnosticCommand">AsyncCommand to use for diagnostics.</param>
        /// <param name="canUseTelemetry">If telemetry can be used.</param>
        internal PSConfigurationProcessor(IConfigurationSetProcessorFactory factory, AsyncCommand diagnosticCommand, bool canUseTelemetry)
        {
            this.Processor = new ConfigurationProcessor(factory);
            this.Processor.MinimumLevel = DiagnosticLevel.Verbose;
            this.Processor.Caller = "Microsoft.WinGet.Configuration";
            this.Processor.Diagnostics += (sender, args) => this.LogConfigurationDiagnostics(args);
            this.Processor.GenerateTelemetryEvents = canUseTelemetry;
            this.diagnosticCommand = diagnosticCommand;
        }

        /// <summary>
        /// Gets the ConfigurationProcessor.
        /// </summary>
        internal ConfigurationProcessor Processor { get; private set; }

        /// <summary>
        /// Updates the cmdlet that is used for diagnostics.
        /// </summary>
        /// <param name="newDiagnosticCommand">New diagnostic command.</param>
        internal void UpdateDiagnosticCmdlet(AsyncCommand newDiagnosticCommand)
        {
            lock (CmdletLock)
            {
                this.diagnosticCommand = newDiagnosticCommand;
            }
        }

        private void LogConfigurationDiagnostics(IDiagnosticInformation diagnosticInformation)
        {
            try
            {
                AsyncCommand asyncCommand = this.diagnosticCommand;
                if (asyncCommand != null)
                {
                    // Printing each diagnostic error in their own equivalent stream is too noisy.
                    // If users want them they have to specify -Verbose.
                    string tag = $"[Diagnostic{diagnosticInformation.Level}] ";
                    asyncCommand.Write(StreamType.Verbose, $"{tag}{diagnosticInformation.Message}");
                }
            }
            catch (Exception)
            {
                // Please don't throw here.
            }
        }
    }
}
