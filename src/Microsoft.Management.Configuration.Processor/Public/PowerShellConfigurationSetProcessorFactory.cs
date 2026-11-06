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
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Factory;
    using Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.PowerShell.Set;
    using Microsoft.Management.Configuration.SetProcessorFactory;
    using static Microsoft.Management.Configuration.Processor.PowerShell.Constants.PowerShellConstants;

    /// <summary>
    /// IConfigurationSetProcessorFactory implementation using PowerShell DSC v2.
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
        private bool useSettingsForModulePath = true;
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
                this.useSettingsForModulePath = false;
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
        /// Gets the unsecured winget module path, including for inherited-path cleanup.
        /// </summary>
        /// <returns>The unsecured winget module path.</returns>
        internal static string GetUnsecuredDefaultWinGetModulePath()
        {
            return Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                    @"Microsoft\WinGet\Configuration\Modules");
        }

        /// <summary>
        /// Gets the default winget module path.
        /// </summary>
        /// <param name="allowSettingsOverride">Whether to allow settings override.</param>
        /// <returns>The default winget module path.</returns>
        internal static (string, bool) GetDefaultWinGetModulePath(bool allowSettingsOverride = false)
        {
            WindowsPackageManagerConfigurationGetModulePath(allowSettingsOverride, out bool fullToken, out string defaultModulePath);
            return (defaultModulePath, fullToken);
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

        /// <summary>
        /// Configures discovery before importing modules, preserving explicit caller paths.
        /// </summary>
        /// <param name="processorEnvironment">The processor environment.</param>
        /// <param name="fullToken">Whether the native runtime selected the full-token context.</param>
        /// <param name="defaultModulePath">The context-specific native default module path.</param>
        /// <param name="effectiveModulePath">The locally resolved effective root, or the built-in default for explicit selections.</param>
        internal void ConfigureModulePaths(IProcessorEnvironment processorEnvironment, bool fullToken, string defaultModulePath, string effectiveModulePath)
        {
            // If the native runtime selected the full-token context, we need to clean up inherited paths to remove the unsecured default winget module path.
            if (fullToken)
            {
                processorEnvironment.CleanupPSModulePath(GetUnsecuredDefaultWinGetModulePath());
            }

            if (this.AdditionalModulePaths is not null)
            {
                processorEnvironment.PrependPSModulePaths(this.AdditionalModulePaths);
            }

            // Always add the context-specific winget path.
            processorEnvironment.PrependPSModulePath(defaultModulePath);
            if (this.Location == PowerShellConfigurationProcessorLocation.WinGetModulePath)
            {
                this.OnDiagnostics(DiagnosticLevel.Verbose, "Using locally resolved winget module path");
                processorEnvironment.SetLocation(PowerShellConfigurationProcessorLocation.Custom, effectiveModulePath);
                if (effectiveModulePath != defaultModulePath)
                {
                    processorEnvironment.PrependPSModulePath(effectiveModulePath);
                }
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
        }

        /// <inheritdoc />
        protected override IConfigurationSetProcessor CreateSetProcessorInternal(ConfigurationSet? set, bool isLimitMode)
        {
            (string defaultModulePath, bool fullToken) = GetDefaultWinGetModulePath(false);
            string settingsModulePath = defaultModulePath;
            if (this.useSettingsForModulePath)
            {
                (settingsModulePath, fullToken) = GetDefaultWinGetModulePath(true);
            }

            var envFactory = new ProcessorEnvironmentFactory(this.ProcessorType);
            var processorEnvironment = envFactory.CreateEnvironment(
                this,
                this.Policy);

            this.ConfigureModulePaths(processorEnvironment, fullToken, defaultModulePath, settingsModulePath);

            this.OnDiagnostics(DiagnosticLevel.Verbose, $"  Effective module path:\n{processorEnvironment.GetVariable<string>(Variables.PSModulePath)}");

            processorEnvironment.ValidateRunspace();

            return new PowerShellConfigurationSetProcessor(processorEnvironment, set, isLimitMode) { SetProcessorFactory = this };
        }

        [DllImport("WindowsPackageManager.dll", ExactSpelling = true, PreserveSig = false)]
        private static extern void WindowsPackageManagerConfigurationGetModulePath(
            [MarshalAs(UnmanagedType.Bool)] bool useSettings,
            [MarshalAs(UnmanagedType.Bool)] out bool fullToken,
            [MarshalAs(UnmanagedType.BStr)] out string path);
    }
}
