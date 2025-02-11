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
    using System.Runtime.CompilerServices;
    using System.Text;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Factory;
    using Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.PowerShell.Set;
    using Microsoft.Management.Configuration.SetProcessorFactory;
    using static Microsoft.Management.Configuration.Processor.PowerShell.Constants.PowerShellConstants;

    /// <summary>
    /// ConfigurationSetProcessorFactory implementation.
    /// </summary>
#if WinGetCsWinRTEmbedded
    internal
#else
    public
#endif
        sealed partial class PowerShellConfigurationSetProcessorFactory : ConfigurationSetProcessorFactoryBase, IConfigurationSetProcessorFactory, IPowerShellConfigurationProcessorFactoryProperties, IPwshConfigurationSetProcessorFactoryProperties
    {
        // Backing variables for properties that are restricted in limit mode.
        private PowerShellConfigurationProcessorType processorType = PowerShellConfigurationProcessorType.Default;
        private IReadOnlyList<string>? additionalModulePaths;
        private IReadOnlyList<string>? implicitModulePaths;
        private PowerShellConfigurationProcessorPolicy policy = PowerShellConfigurationProcessorPolicy.Default;
        private PowerShellConfigurationProcessorLocation location = PowerShellConfigurationProcessorLocation.Default;
        private string? customLocation;

        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellConfigurationSetProcessorFactory"/> class.
        /// </summary>
        public PowerShellConfigurationSetProcessorFactory()
        {
        }

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

                // Create a copy of incoming value
                List<string> newModulePaths = new List<string>();
                if (value != null)
                {
                    foreach (string path in value)
                    {
                        newModulePaths.Add(path);
                    }
                }

                // Add implicit module paths if applicable
                if (this.implicitModulePaths != null)
                {
                    foreach (string path in this.implicitModulePaths)
                    {
                        if (!newModulePaths.Contains(path))
                        {
                            newModulePaths.Add(path);
                        }
                    }
                }

                this.additionalModulePaths = newModulePaths;
            }
        }

        /// <summary>
        /// Gets or sets the implicit module paths. These paths are always included in AdditionalModulePaths.
        /// </summary>
        public IReadOnlyList<string>? ImplicitModulePaths
        {
            get
            {
                return this.implicitModulePaths;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting ImplicitModulePaths in limit mode is invalid.");
                }

                this.implicitModulePaths = value;

                // Apply to additional module paths if applicable.
                if (this.implicitModulePaths != null)
                {
                    List<string> newModulePaths = new List<string>();
                    if (this.additionalModulePaths != null)
                    {
                        foreach (string path in this.additionalModulePaths)
                        {
                            newModulePaths.Add(path);
                        }
                    }

                    foreach (string path in this.implicitModulePaths)
                    {
                        if (!newModulePaths.Contains(path))
                        {
                            newModulePaths.Add(path);
                        }
                    }

                    this.additionalModulePaths = newModulePaths;
                }
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
        /// Gets or sets the configuration policy.
        /// </summary>
        PwshConfigurationProcessorPolicy IPwshConfigurationSetProcessorFactoryProperties.Policy
        {
            get { return Helpers.TypeHelpers.ToPwshConfigurationProcessorPolicy(this.Policy); }
            set { this.Policy = Helpers.TypeHelpers.ToPowerShellConfigurationProcessorPolicy(value); }
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
        /// Gets or sets the module location.
        /// </summary>
        PwshConfigurationProcessorLocation IPwshConfigurationSetProcessorFactoryProperties.Location
        {
            get { return Helpers.TypeHelpers.ToPwshConfigurationProcessorLocation(this.Location); }
            set { this.Location = Helpers.TypeHelpers.ToPowerShellConfigurationProcessorLocation(value); }
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
        /// Sends diagnostic if appropriate for PowerShell streams.
        /// </summary>
        /// <param name="level">The level of this diagnostic message.</param>
        /// <param name="pwsh">The PowerShell object.</param>
        internal void OnDiagnostics(DiagnosticLevel level, System.Management.Automation.PowerShell pwsh)
        {
            if (this.AreDiagnosticsEnabled() && level >= this.MinimumLevel && pwsh.HadErrors)
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

                this.OnDiagnostics(level, builder.ToString());
            }
        }

        /// <inheritdoc />
        protected override IConfigurationSetProcessor CreateSetProcessorInternal(ConfigurationSet? set, bool isLimitMode)
        {
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

            return new PowerShellConfigurationSetProcessor(processorEnvironment, set, isLimitMode) { SetProcessorFactory = this };
        }
    }
}
