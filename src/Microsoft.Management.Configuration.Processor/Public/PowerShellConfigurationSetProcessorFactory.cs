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
    using System.Reflection;
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
        private const string ExternalModulesName = "ExternalModules";

        private readonly ConfigurationSet? limitationSet;

        // Backing variables for properties that are restricted in limit mode.
        private PowerShellConfigurationProcessorType processorType = PowerShellConfigurationProcessorType.Default;
        private IReadOnlyList<string>? additionalModulePaths;
        private PowerShellConfigurationProcessorPolicy policy = PowerShellConfigurationProcessorPolicy.Default;
        private PowerShellConfigurationProcessorLocation location = PowerShellConfigurationProcessorLocation.Default;
        private string? customLocation;

        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellConfigurationSetProcessorFactory"/> class.
        /// </summary>
        /// <param name="limitationSet">Limitation Configuration Set.</param>
        public PowerShellConfigurationSetProcessorFactory(ConfigurationSet? limitationSet = null)
        {
            this.limitationSet = limitationSet;

            if (this.IsLimitMode())
            {
                // Set default properties.
                // This should be consistent with what WindowsPackageManagerConfigurationCompleteOutOfProcessFactoryInitialization
                // sets for initialization.
                var externalModulesPath = GetExternalModulesPath();
                if (!string.IsNullOrWhiteSpace(externalModulesPath))
                {
                    this.additionalModulePaths = new List<string>() { externalModulesPath };
                }

                this.processorType = PowerShellConfigurationProcessorType.Hosted;
            }
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
        public PowerShellConfigurationProcessorType ProcessorType
        {
            get
            {
                return this.processorType;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting ProcessorType in limit mode is invalid.");
                }

                this.processorType = value;
            }
        }

        /// <summary>
        /// Gets or sets the additional module paths.
        /// </summary>
        public IReadOnlyList<string>? AdditionalModulePaths
        {
            get
            {
                return this.additionalModulePaths;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting AdditionalModulePaths in limit mode is invalid.");
                }

                this.additionalModulePaths = value;
            }
        }

        /// <summary>
        /// Gets or sets the configuration policy.
        /// </summary>
        public PowerShellConfigurationProcessorPolicy Policy
        {
            get
            {
                return this.policy;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting Policy in limit mode is invalid.");
                }

                this.policy = value;
            }
        }

        /// <summary>
        /// Gets or sets the module location.
        /// </summary>
        public PowerShellConfigurationProcessorLocation Location
        {
            get
            {
                return this.location;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting Location in limit mode is invalid.");
                }

                this.location = value;
            }
        }

        /// <summary>
        /// Gets or sets the install module path. Only used for Scope = Custom.
        /// </summary>
        public string? CustomLocation
        {
            get
            {
                return this.customLocation;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting CustomLocation in limit mode is invalid.");
                }

                this.customLocation = value;
            }
        }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="incomingSet">Configuration Set.</param>
        /// <returns>Configuration set processor.</returns>
        public IConfigurationSetProcessor CreateSetProcessor(ConfigurationSet? incomingSet)
        {
            try
            {
                this.OnDiagnostics(DiagnosticLevel.Informational, $"The set processor factory is running in limit mode: {this.IsLimitMode()}.");

                ConfigurationSet? set = this.IsLimitMode() ? this.limitationSet : incomingSet;

                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Creating set processor for `{set?.Name ?? "<null>"}`...");

                if (set != null && (set.Parameters.Count > 0 || set.Variables.Count > 0))
                {
                    this.OnDiagnostics(DiagnosticLevel.Error, $"  Parameters/variables are not yet supported.");
                    throw new NotImplementedException();
                }

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

                return new ConfigurationSetProcessor(processorEnvironment, set, this.IsLimitMode()) { SetProcessorFactory = this };
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

        private static string GetExternalModulesPath()
        {
            var currentAssemblyDirectoryPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            if (currentAssemblyDirectoryPath != null)
            {
                var packageRootPath = Directory.GetParent(currentAssemblyDirectoryPath)?.FullName;
                if (packageRootPath != null)
                {
                    return Path.Combine(packageRootPath, ExternalModulesName);
                }
            }

            return string.Empty;
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

        private bool IsLimitMode()
        {
            return this.limitationSet != null;
        }
    }
}
