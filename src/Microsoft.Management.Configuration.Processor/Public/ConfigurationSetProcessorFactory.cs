// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProcessorFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using System;
    using System.Management.Automation;
    using System.Text;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Set;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// ConfigurationSetProcessorFactory implementation.
    /// </summary>
    public sealed class ConfigurationSetProcessorFactory : IConfigurationSetProcessorFactory
    {
        private readonly ConfigurationProcessorType type;
        private readonly IConfigurationProcessorFactoryProperties? properties;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProcessorFactory"/> class.
        /// </summary>
        /// <param name="type">Type.</param>
        /// <param name="properties">Properties.</param>
        public ConfigurationSetProcessorFactory(ConfigurationProcessorType type, IConfigurationProcessorFactoryProperties? properties)
        {
            this.type = type;
            this.properties = properties;
        }

        /// <summary>
        /// Diagnostics event; useful for logging and/or verbose output.
        /// </summary>
        public event EventHandler<DiagnosticInformation>? Diagnostics;

        /// <summary>
        /// Gets or sets the minimum diagnostic level to send.
        /// </summary>
        public DiagnosticLevel MinimumLevel { get; set; } = DiagnosticLevel.Informational;

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

                var envFactory = new ProcessorEnvironmentFactory(this.type);
                var processorEnvironment = envFactory.CreateEnvironment(this);

                if (this.properties is not null)
                {
                    var additionalPsModulePaths = this.properties.AdditionalModulePaths;
                    if (additionalPsModulePaths is not null)
                    {
                        processorEnvironment.PrependPSModulePaths(additionalPsModulePaths);
                    }
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
        /// Sends diagnostics if appropriate.
        /// </summary>
        /// <param name="level">The level of this diagnostic message.</param>
        /// <param name="message">The diagnostic message.</param>
        internal void OnDiagnostics(DiagnosticLevel level, string message)
        {
            EventHandler<DiagnosticInformation>? diagnostics = this.Diagnostics;
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
            EventHandler<DiagnosticInformation>? diagnostics = this.Diagnostics;
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

        private void InvokeDiagnostics(EventHandler<DiagnosticInformation> diagnostics, DiagnosticLevel level, string message)
        {
            DiagnosticInformation information = new ()
            {
                Level = level,
                Message = message,
            };
            diagnostics.Invoke(this, information);
        }
    }
}