// -----------------------------------------------------------------------------
// <copyright file="PowerShellConfigurationSetProcessorFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Management.Automation;
    using System.Text;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Set;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// ConfigurationSetProcessorFactory implementation.
    /// </summary>
    public sealed class PowerShellConfigurationSetProcessorFactory : IConfigurationSetProcessorFactory, IPowerShellConfigurationProcessorFactoryProperties
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellConfigurationSetProcessorFactory"/> class.
        /// </summary>
        public PowerShellConfigurationSetProcessorFactory()
        {
        }

        /// <summary>
        /// Diagnostics event; useful for logging and/or verbose output.
        /// </summary>
        public event EventHandler<IDiagnosticInformation>? Diagnostics;

        /// <summary>
        /// Gets or sets the minimum diagnostic level to send.
        /// </summary>
        public DiagnosticLevel MinimumLevel { get; set; } = DiagnosticLevel.Informational;

        /// <summary>
        /// Gets or sets the processor type.
        /// </summary>
        public PowerShellConfigurationProcessorType ProcessorType { get; set; } = PowerShellConfigurationProcessorType.Default;

        /// <summary>
        /// Gets or sets the additional module paths.
        /// </summary>
        public IReadOnlyList<string>? AdditionalModulePaths { get; set; }

        /// <summary>
        /// Gets or sets the configuration policy.
        /// </summary>
        public PowerShellConfigurationProcessorPolicy Policy { get; set; } = PowerShellConfigurationProcessorPolicy.Default;

        /// <summary>
        /// Gets or sets the module scope.
        /// </summary>
        public PowerShellConfigurationProcessorLocation Location { get; set; } = PowerShellConfigurationProcessorLocation.Default;

        /// <summary>
        /// Gets or sets the install module path. Only used for Scope = Custom.
        /// </summary>
        public string? CustomLocation { get; set; }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="set">Configuration Set.</param>
        /// <returns>Configuration set processor.</returns>
        public IConfigurationSetProcessor CreateSetProcessor(ConfigurationSet set)
        {
            try
            {
                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Creating set processor for `{set.Name}`...");

                var envFactory = new ProcessorEnvironmentFactory(this.ProcessorType);
                var processorEnvironment = envFactory.CreateEnvironment(
                    this,
                    this.Policy);

                if (this.AdditionalModulePaths is not null)
                {
                    processorEnvironment.PrependPSModulePaths(this.AdditionalModulePaths);
                }

                // Always add the winget path.
                var wingetModulePath = GetWinGetModulePath();
                processorEnvironment.PrependPSModulePath(wingetModulePath);
                if (this.Location == PowerShellConfigurationProcessorLocation.WinGetModulePath)
                {
                    this.OnDiagnostics(DiagnosticLevel.Verbose, "Using winget module path");
                    processorEnvironment.SetLocation(PowerShellConfigurationProcessorLocation.Custom, wingetModulePath);
                }
                else if (this.Location == PowerShellConfigurationProcessorLocation.Custom)
                {
                    if (string.IsNullOrEmpty(this.CustomLocation))
                    {
                        throw new ArgumentNullException(nameof(this.CustomLocation));
                    }

                    processorEnvironment.SetLocation(this.Location, this.CustomLocation);
                    processorEnvironment.PrependPSModulePath(this.CustomLocation);
                }
                else
                {
                    processorEnvironment.SetLocation(this.Location, null);
                }

                this.OnDiagnostics(DiagnosticLevel.Verbose, $"  Effective module path:\n{processorEnvironment.GetVariable<string>(Variables.PSModulePath)}");

                processorEnvironment.ValidateRunspace();

                this.OnDiagnostics(DiagnosticLevel.Verbose, "... done creating set processor.");

                return new ConfigurationSetProcessor(processorEnvironment, set) { SetProcessorFactory = this };
            }
            catch (Exception ex)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, ex.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the winget module path.
        /// </summary>
        /// <returns>The winget module path.</returns>
        internal static string GetWinGetModulePath()
        {
            return Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    @"Microsoft\WinGet\Configuration\Modules");
        }

        /// <summary>
        /// Sends diagnostics if appropriate.
        /// </summary>
        /// <param name="level">The level of this diagnostic message.</param>
        /// <param name="message">The diagnostic message.</param>
        internal void OnDiagnostics(DiagnosticLevel level, string message)
        {
            EventHandler<IDiagnosticInformation>? diagnostics = this.Diagnostics;
            if (diagnostics != null && level >= this.MinimumLevel)
            {
                this.InvokeDiagnostics(diagnostics, level, message);
            }
        }

        /// <summary>
        /// Sends diagnostic if appropriate for PowerShell streams.
        /// </summary>
        /// <param name="level">The level of this diagnostic message.</param>
        /// <param name="pwsh">The PowerShell object.</param>
        internal void OnDiagnostics(DiagnosticLevel level, PowerShell pwsh)
        {
            EventHandler<IDiagnosticInformation>? diagnostics = this.Diagnostics;
            if (diagnostics != null && level >= this.MinimumLevel && pwsh.HadErrors)
            {
                var builder = new StringBuilder();

                // There are the last commands ran by that PowerShell obj, not all in our session.
                builder.Append("PowerShellCommands: ");
                foreach (var c in pwsh.Commands.Commands)
                {
                    builder.Append($"['{c.CommandText}'");
                    if (c.Parameters.Count > 0)
                    {
                        builder.Append(" Parameters: ");
                        foreach (var p in c.Parameters)
                        {
                            builder.Append($"{p.Name} = '{p.Value}' ");
                        }

                        builder.Append("]");
                    }

                    builder.AppendLine();
                }

                foreach (var error in pwsh.Streams.Error)
                {
                    builder.AppendLine($"[WriteError] {error}");
                }

                this.InvokeDiagnostics(diagnostics, level, builder.ToString());
            }
        }

        private void InvokeDiagnostics(EventHandler<IDiagnosticInformation> diagnostics, DiagnosticLevel level, string message)
        {
            Helpers.DiagnosticInformation information = new ()
            {
                Level = level,
                Message = message,
            };
            diagnostics.Invoke(this, information);
        }
    }
}