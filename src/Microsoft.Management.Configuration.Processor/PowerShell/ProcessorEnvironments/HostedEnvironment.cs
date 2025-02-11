// -----------------------------------------------------------------------------
// <copyright file="HostedEnvironment.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.Runspaces
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Runtime.InteropServices.WindowsRuntime;
    using Microsoft.Management.Configuration.Processor.Constants;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscModules;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;
    using Windows.Security.Cryptography.Certificates;
    using Windows.Storage.Streams;
    using static Microsoft.Management.Configuration.Processor.PowerShell.Constants.PowerShellConstants;

    /// <summary>
    /// Process environment. Provides interaction with PowerShell for a hosted environment.
    /// </summary>
    internal class HostedEnvironment : IProcessorEnvironment
    {
        private readonly PowerShellConfigurationProcessorType type;
        private readonly IPowerShellGet powerShellGet;

        private PowerShellConfigurationProcessorLocation location = PowerShellConfigurationProcessorLocation.CurrentUser;
        private string? customLocation;

        /// <summary>
        /// Initializes a new instance of the <see cref="HostedEnvironment"/> class.
        /// </summary>
        /// <param name="runspace">PowerShell Runspace.</param>
        /// <param name="type">Configuration processor type.</param>
        /// <param name="dscModule">IDscModule.</param>
        public HostedEnvironment(
            Runspace runspace,
            PowerShellConfigurationProcessorType type,
            IDscModule dscModule)
        {
            this.Runspace = runspace;
            this.type = type;
            this.DscModule = dscModule;

            // TODO: once v3 is release implement v3 version.
            this.powerShellGet = new PowerShellGetV2();
        }

        /// <inheritdoc/>
        public Runspace Runspace { get; }

        /// <summary>
        /// Gets the DscModule.
        /// </summary>
        internal IDscModule DscModule { get; }

        /// <summary>
        /// Gets or initializes the set processor factory.
        /// </summary>
        internal PowerShellConfigurationSetProcessorFactory? SetProcessorFactory { get; init; }

        /// <inheritdoc/>
        public void ValidateRunspace()
        {
            // Only support PowerShell Core.
            if (this.GetVariable<string>(Variables.PSEdition) != Core)
            {
                throw new NotSupportedException("Only PowerShell Core is supported.");
            }

            // If opening a runspace has failures, like one of the modules in ImportPSModule is not found, it won't throw but
            // write to the error output. This is not a fatal error, since we install PSDesiredStateConfiguration
            // module if not found, so unless there's a real reason keep it in verbose.
            var errors = this.GetVariable<ArrayList>(Variables.Error);
            if (errors.Count > 0)
            {
                this.OnDiagnostics(
                    DiagnosticLevel.Verbose,
                    $"Error creating runspace '{string.Join("\n", errors.Cast<string>().ToArray())}'");
            }

            var powerShellGet = PowerShellHelpers.CreateModuleSpecification(
                    Modules.PowerShellGet,
                    minVersion: Modules.PowerShellGetMinVersion);
            if (!this.ValidateModule(powerShellGet))
            {
                var previousVersion = this.GetAvailableModule(
                    PowerShellHelpers.CreateModuleSpecification(
                        Modules.PowerShellGet));
                string message = $"Required '{powerShellGet}'";
                if (previousVersion is not null)
                {
                    message += $" Found '{previousVersion.Name} {previousVersion.Version}'";
                }

                throw new NotSupportedException(message);
            }

            // Make sure PSDesiredConfiguration is present.
            this.InstallModule(this.DscModule.ModuleSpecification);
        }

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetAllDscResources()
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            var results = this.DscModule.GetAllDscResources(pwsh);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return results;
        }

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetDscResourcesInModule(ModuleSpecification moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            var results = this.DscModule.GetDscResourcesInModule(pwsh, moduleSpecification);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return results;
        }

        /// <inheritdoc/>
        public DscResourceInfoInternal? GetDscResource(ConfigurationUnitAndModule unitInternal)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            var result = this.DscModule.GetDscResource(pwsh, unitInternal.ResourceName, unitInternal.Module);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return result;
        }

        /// <inheritdoc/>
        public ValueSet InvokeGetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            var result = this.DscModule.InvokeGetResource(pwsh, settings, name, moduleSpecification);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return result;
        }

        /// <inheritdoc/>
        public bool InvokeTestResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            var result = this.DscModule.InvokeTestResource(pwsh, settings, name, moduleSpecification);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return result;
        }

        /// <inheritdoc/>
        public bool InvokeSetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            var result = this.DscModule.InvokeSetResource(pwsh, settings, name, moduleSpecification);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return result;
        }

        /// <inheritdoc/>
        public PSModuleInfo? GetImportedModule(ModuleSpecification moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var moduleInfo = pwsh.AddCommand(Commands.GetModule)
                                 .AddParameter(Parameters.FullyQualifiedName, moduleSpecification)
                                 .Invoke<PSModuleInfo>()
                                 .FirstOrDefault();

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return moduleInfo;
        }

        /// <inheritdoc/>
        public PSModuleInfo? GetAvailableModule(ModuleSpecification moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var moduleInfo = pwsh.AddCommand(Commands.GetModule)
                                 .AddParameter(Parameters.FullyQualifiedName, moduleSpecification)
                                 .AddParameter(Parameters.ListAvailable)
                                 .Invoke<PSModuleInfo>()
                                 .FirstOrDefault();

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return moduleInfo;
        }

        /// <inheritdoc/>
        public PSModuleInfo? GetAvailableModule(string path)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var moduleInfo = pwsh.AddCommand(Commands.GetModule)
                                 .AddParameter(Parameters.Name, path)
                                 .AddParameter(Parameters.ListAvailable)
                                 .Invoke<PSModuleInfo>()
                                 .FirstOrDefault();

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return moduleInfo;
        }

        /// <inheritdoc/>
        public void ImportModule(ModuleSpecification moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            _ = pwsh.AddCommand(Commands.ImportModule)
                    .AddParameter(Parameters.FullyQualifiedName, moduleSpecification)
                    .Invoke();

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
        }

        /// <inheritdoc/>
        public void ImportModule(string path)
        {
            if (!File.Exists(path))
            {
                throw new FileNotFoundException(path);
            }

            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            _ = pwsh.AddCommand(Commands.ImportModule)
                    .AddParameter(Parameters.Name, path)
                    .Invoke();

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
        }

        /// <inheritdoc/>
        public PSObject? GetInstalledModule(ModuleSpecification moduleSpecification)
        {
            // Instead of Get-InstalledModule, we look for PSGetModuleInfo.xml and serialize it
            // if found. This allow us to get the information from Install-Module and Save-Module.
            var module = this.GetAvailableModule(moduleSpecification);
            if (module is null)
            {
                return null;
            }

            var getModuleInfoFile = Path.Combine(module.ModuleBase, "PSGetModuleInfo.xml");
            if (!File.Exists(getModuleInfoFile))
            {
                // Keep Get-InstalledModule behaviour.
                return null;
            }

            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            var installedModule = pwsh.AddCommand(Commands.ImportCliXml)
                                      .AddParameter(Parameters.Path, getModuleInfoFile)
                                      .Invoke()
                                      .FirstOrDefault();

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return installedModule;
        }

        /// <inheritdoc/>
        public PSObject? FindModule(ConfigurationUnitInternal unitInternal)
        {
            // Don't use ModuleSpecification here. Each parameter is independent and
            // we need version even if a module was not specified.
            string? moduleName = unitInternal.GetDirective<string>(DirectiveConstants.Module);

            if (string.IsNullOrEmpty(moduleName))
            {
                return null;
            }

            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var result = this.powerShellGet.FindModule(
                pwsh,
                moduleName,
                unitInternal.GetSemanticVersion(),
                unitInternal.GetSemanticMinVersion(),
                unitInternal.GetSemanticMaxVersion(),
                unitInternal.GetDirective<string>(DirectiveConstants.Repository),
                unitInternal.GetDirective(DirectiveConstants.AllowPrerelease));

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return result;
        }

        /// <inheritdoc/>
        public PSObject? FindDscResource(ConfigurationUnitInternal unitInternal)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var result = this.powerShellGet.FindDscResource(
                pwsh,
                unitInternal.ResourceName,
                unitInternal.GetDirective<string>(DirectiveConstants.Module),
                unitInternal.GetSemanticVersion(),
                unitInternal.GetSemanticMinVersion(),
                unitInternal.GetSemanticMaxVersion(),
                unitInternal.GetDirective<string>(DirectiveConstants.Repository),
                unitInternal.GetDirective(DirectiveConstants.AllowPrerelease));

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return result;
        }

        /// <inheritdoc/>
        public void SaveModule(PSObject inputObject, string location)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            this.powerShellGet.SaveModule(pwsh, inputObject, location);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
        }

        /// <inheritdoc/>
        public void SaveModule(ModuleSpecification moduleSpecification, string location)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);
            this.powerShellGet.SaveModule(pwsh, moduleSpecification, location);
            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
        }

        /// <inheritdoc/>
        public void InstallModule(PSObject inputObject)
        {
            if (this.location == PowerShellConfigurationProcessorLocation.Custom)
            {
                if (string.IsNullOrEmpty(this.customLocation))
                {
                    throw new ArgumentNullException(nameof(this.customLocation));
                }

                this.SaveModule(inputObject, this.customLocation);
            }
            else
            {
                using PowerShell pwsh = PowerShell.Create(this.Runspace);
                this.powerShellGet.InstallModule(pwsh, inputObject, this.location == PowerShellConfigurationProcessorLocation.AllUsers);
                this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            }
        }

        /// <inheritdoc/>
        public void InstallModule(ModuleSpecification moduleSpecification)
        {
            // Maybe is already there.
            if (!this.ValidateModule(moduleSpecification))
            {
                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Installing module: {moduleSpecification.Name} ...");

                // Ok, we have to get it.
                if (this.location == PowerShellConfigurationProcessorLocation.Custom)
                {
                    if (string.IsNullOrEmpty(this.customLocation))
                    {
                        throw new ArgumentNullException(nameof(this.customLocation));
                    }

                    this.OnDiagnostics(DiagnosticLevel.Verbose, $"... calling save module ...");
                    this.SaveModule(moduleSpecification, this.customLocation);
                }
                else
                {
                    this.OnDiagnostics(DiagnosticLevel.Verbose, $"... calling install module ...");
                    using PowerShell pwsh = PowerShell.Create(this.Runspace);
                    this.powerShellGet.InstallModule(pwsh, moduleSpecification, this.location == PowerShellConfigurationProcessorLocation.AllUsers);
                    this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
                }

                this.OnDiagnostics(DiagnosticLevel.Verbose, $" ... module installed.");
            }
        }

        /// <inheritdoc/>
        public List<Certificate> GetCertsOfValidSignedFiles(string[] paths)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var signatures = pwsh.AddCommand(Commands.GetChildItem)
                                 .AddParameter(Parameters.Path, paths)
                                 .AddCommand(Commands.GetAuthenticodeSignature)
                                 .Invoke<Signature>();

            var thumbprint = new HashSet<string>();
            var certificates = new List<Certificate>();
            foreach (var signature in signatures)
            {
                if (signature.Status == SignatureStatus.Valid)
                {
                    if (thumbprint.Add(signature.SignerCertificate.Thumbprint))
                    {
                        IBuffer buffer = signature.SignerCertificate.GetRawCertData().AsBuffer();
                        certificates.Add(new Certificate(buffer));
                    }
                }
            }

            this.OnDiagnostics(DiagnosticLevel.Verbose, pwsh);
            return certificates;
        }

        /// <inheritdoc/>
        public TType GetVariable<TType>(string name)
        {
            return (TType)this.Runspace.SessionStateProxy.PSVariable.GetValue(name);
        }

        /// <inheritdoc/>
        public void SetVariable(string name, object value)
        {
            this.Runspace.SessionStateProxy.PSVariable.Set(name, value);
        }

        /// <inheritdoc/>
        public void SetPSModulePath(string path)
        {
            this.SetVariable(Variables.PSModulePath, path);
        }

        /// <inheritdoc/>
        public void SetPSModulePaths(IReadOnlyList<string> paths)
        {
            this.SetVariable(Variables.PSModulePath, string.Join(";", paths));
        }

        /// <inheritdoc/>
        public void PrependPSModulePath(string path)
        {
            var oldModulePath = this.GetModulePaths();
            if (!oldModulePath.Contains(path))
            {
                this.SetPSModulePath($"{path};{string.Join(";", oldModulePath)}");
            }
        }

        /// <inheritdoc/>
        public void PrependPSModulePaths(IReadOnlyList<string> paths)
        {
            var newPaths = paths.ToList();
            var oldModulePath = this.GetModulePaths();
            foreach (var newPath in paths)
            {
                if (oldModulePath.Contains(newPath))
                {
                    newPaths.Remove(newPath);
                }
            }

            if (newPaths.Any())
            {
                this.SetPSModulePath($"{string.Join(";", newPaths)};{string.Join(";", oldModulePath)}");
            }
        }

        /// <inheritdoc/>
        public void AppendPSModulePath(string path)
        {
            var oldModulePath = this.GetModulePaths();
            if (!oldModulePath.Contains(path))
            {
                this.SetPSModulePath($"{string.Join(";", oldModulePath)};{path}");
            }
        }

        /// <inheritdoc/>
        public void AppendPSModulePaths(IReadOnlyList<string> paths)
        {
            var newPaths = paths.ToList();
            var oldModulePath = this.GetModulePaths();
            foreach (var newPath in paths)
            {
                if (oldModulePath.Contains(newPath))
                {
                    newPaths.Remove(newPath);
                }
            }

            if (newPaths.Any())
            {
                this.SetPSModulePath($"{string.Join(";", oldModulePath)};{string.Join(";", newPaths)}");
            }
        }

        /// <inheritdoc/>
        public void CleanupPSModulePath(string path)
        {
            string newModulePath = this.GetVariable<string>(Variables.PSModulePath)
                                       .Replace($"{path};", null)
                                       .Replace($";{path}", null);

            this.SetPSModulePath(newModulePath);
        }

        /// <inheritdoc/>
        public void SetLocation(PowerShellConfigurationProcessorLocation location, string? customLocation)
        {
            this.location = location;
            if (this.location == PowerShellConfigurationProcessorLocation.Custom)
            {
                if (string.IsNullOrEmpty(customLocation))
                {
                    throw new ArgumentNullException(nameof(customLocation));
                }

                this.customLocation = customLocation;
            }
        }

        private bool ValidateModule(ModuleSpecification moduleSpecification)
        {
            this.OnDiagnostics(DiagnosticLevel.Verbose, $"Validating module: {moduleSpecification.Name} ...");

            var loadedModule = this.GetImportedModule(moduleSpecification);
            if (loadedModule is not null)
            {
                this.OnDiagnostics(DiagnosticLevel.Verbose, $" ... module is already imported.");
                return true;
            }

            var availableModule = this.GetAvailableModule(moduleSpecification);
            if (availableModule is not null)
            {
                this.OnDiagnostics(DiagnosticLevel.Verbose, $" ... module is available, importing ...");
                this.ImportModule(moduleSpecification);
                this.OnDiagnostics(DiagnosticLevel.Verbose, $" ... module imported.");
                return true;
            }

            this.OnDiagnostics(DiagnosticLevel.Verbose, $" ... module not found.");
            return false;
        }

        private void OnDiagnostics(DiagnosticLevel level, PowerShell pwsh)
        {
            this.SetProcessorFactory?.OnDiagnostics(level, pwsh);
        }

        private void OnDiagnostics(DiagnosticLevel level, string message)
        {
            this.SetProcessorFactory?.OnDiagnostics(level, message);
        }

        private HashSet<string> GetModulePaths()
        {
            return this.GetVariable<string>(Variables.PSModulePath).Split(";").ToHashSet<string>();
        }
    }
}
