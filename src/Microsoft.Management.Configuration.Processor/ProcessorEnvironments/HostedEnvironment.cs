// -----------------------------------------------------------------------------
// <copyright file="HostedEnvironment.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Runspaces
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using System.Runtime.InteropServices.WindowsRuntime;
    using Microsoft.Management.Configuration.Processor.Constants;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;
    using Windows.Security.Cryptography.Certificates;
    using Windows.Storage.Streams;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// Process environment. Provides interaction with PowerShell for a hosted environment.
    /// </summary>
    internal class HostedEnvironment : IProcessorEnvironment
    {
        private ConfigurationProcessorType type;

        /// <summary>
        /// Initializes a new instance of the <see cref="HostedEnvironment"/> class.
        /// </summary>
        /// <param name="runspace">PowerShell Runspace.</param>
        /// <param name="type">Configuration processor type.</param>
        /// <param name="dscModule">IDscModule.</param>
        public HostedEnvironment(Runspace runspace, ConfigurationProcessorType type, IDscModule dscModule)
        {
            this.Runspace = runspace;
            this.type = type;
            this.DscModule = dscModule;
        }

        /// <inheritdoc/>
        public Runspace Runspace { get; }

        /// <summary>
        /// Gets the DscModule.
        /// </summary>
        protected IDscModule DscModule { get; }

        /// <inheritdoc/>
        public void ValidateRunspace()
        {
            // Only support PowerShell Core.
            if (this.GetVariable<string>(Variables.PSEdition) != Core)
            {
                throw new NotSupportedException();
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
        public IReadOnlyList<DscResourceInfoInternal> GetAllDscResources() =>
            this.DscModule.GetAllDscResources(this.Runspace);

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetDscResourcesInModule(ModuleSpecification moduleSpecification) =>
            this.DscModule.GetDscResourcesInModule(this.Runspace, moduleSpecification);

        /// <inheritdoc/>
        public DscResourceInfoInternal? GetDscResource(ConfigurationUnitInternal unitInternal) =>
            this.DscModule.GetDscResource(this.Runspace, unitInternal.Unit.UnitName, unitInternal.Module);

        /// <inheritdoc/>
        public ValueSet InvokeGetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification) =>
            this.DscModule.InvokeGetResource(this.Runspace, settings, name, moduleSpecification);

        /// <inheritdoc/>
        public bool InvokeTestResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification) =>
            this.DscModule.InvokeTestResource(this.Runspace, settings, name, moduleSpecification);

        /// <inheritdoc/>
        public bool InvokeSetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification) =>
            this.DscModule.InvokeSetResource(this.Runspace, settings, name, moduleSpecification);

        /// <inheritdoc/>
        public PSModuleInfo? GetImportedModule(ModuleSpecification moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var moduleInfo = pwsh.AddCommand(Commands.GetModule)
                                 .AddParameter(Parameters.FullyQualifiedName, moduleSpecification)
                                 .Invoke<PSModuleInfo>()
                                 .FirstOrDefault();

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

            return moduleInfo;
        }

        /// <inheritdoc/>
        public void ImportModule(ModuleSpecification moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            _ = pwsh.AddCommand(Commands.ImportModule)
                    .AddParameter(Parameters.FullyQualifiedName, moduleSpecification)
                    .Invoke();
        }

        /// <inheritdoc/>
        public PSObject? GetInstalledModule(ModuleSpecification moduleSpecification)
        {
            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, moduleSpecification.Name },
            };

            if (moduleSpecification.Version is not null)
            {
                parameters.Add(Parameters.MinimumVersion, moduleSpecification.Version);
            }

            if (moduleSpecification.MaximumVersion is not null)
            {
                parameters.Add(Parameters.MaximumVersion, moduleSpecification.MaximumVersion);
            }

            if (moduleSpecification.RequiredVersion is not null)
            {
                parameters.Add(Parameters.RequiredVersion, moduleSpecification.RequiredVersion);
            }

            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var result = pwsh.AddCommand(Commands.GetInstalledModule)
                             .AddParameters(parameters)
                             .Invoke()
                             .FirstOrDefault();

            return result;
        }

        /// <inheritdoc/>
        public PSObject? FindDscResource(ConfigurationUnitInternal unitInternal)
        {
            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, unitInternal.Unit.UnitName },
            };

            if (unitInternal.Module is not null)
            {
                parameters.Add(Parameters.ModuleName, unitInternal.Module.Name);

                if (unitInternal.Module.Version is not null)
                {
                    parameters.Add(Parameters.MinimumVersion, unitInternal.Module.Version);
                }

                if (unitInternal.Module.MaximumVersion is not null)
                {
                    parameters.Add(Parameters.MaximumVersion, unitInternal.Module.MaximumVersion);
                }

                if (unitInternal.Module.RequiredVersion is not null)
                {
                    parameters.Add(Parameters.RequiredVersion, unitInternal.Module.RequiredVersion);
                }
            }

            string? repository = unitInternal.GetDirective(DirectiveConstants.Repository);
            if (!string.IsNullOrEmpty(repository))
            {
                parameters.Add(Parameters.Repository, repository);
            }

            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            // TODO: Implement prerelease directive.
            // The result is just a PSCustomObject with a type name of Microsoft.PowerShell.Commands.PSGetDscResourceInfo.
            // When no module is passed and a resource is not found, this will return an empty list. If a module
            // is specified and no resource is found then it will fail earlier because of a Write-Error.
            var result = pwsh.AddCommand(Commands.FindDscResource)
                             .AddParameters(parameters)
                             .Invoke()
                             .FirstOrDefault();

            return result;
        }

        /// <inheritdoc/>
        public void SaveModule(PSObject inputObject, string location)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            _ = pwsh.AddCommand(Commands.SaveModule)
                    .AddParameter(Parameters.Path, location)
                    .AddParameter(Parameters.InputObject, inputObject)
                    .InvokeAndStopOnError();
        }

        /// <inheritdoc/>
        public void InstallModule(PSObject inputObject)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            // If the repository is untrusted, it will fail with:
            //   Microsoft.PowerShell.Commands.WriteErrorException : Exception calling "ShouldContinue" with "5"
            //   argument(s): "A command that prompts the user failed because the host program or the command type
            //   does not support user interaction.
            // If its trusted, PowerShellGets adds the Force parameter to the call to PackageManager\Install-Package.
            // TODO: Once we have policies, we should remove Force. For hosted environments and depending
            // on the policy we will trust PSGallery when we create the Runspace or add Force here.
            _ = pwsh.AddCommand(Commands.InstallModule)
                    .AddParameter(Parameters.InputObject, inputObject)
                    .AddParameter(Parameters.Force)
                    .InvokeAndStopOnError();
        }

        /// <inheritdoc/>
        public void InstallModule(ModuleSpecification moduleSpecification)
        {
            // Maybe is already there.
            if (!this.ValidateModule(moduleSpecification))
            {
                // Ok, we have to get it.
                var parameters = new Dictionary<string, object>()
                {
                    { Parameters.Name, moduleSpecification.Name },
                };
                if (moduleSpecification.Version is not null)
                {
                    parameters.Add(Parameters.MinimumVersion, moduleSpecification.Version);
                }

                if (moduleSpecification.MaximumVersion is not null)
                {
                    parameters.Add(Parameters.MaximumVersion, moduleSpecification.MaximumVersion);
                }

                if (moduleSpecification.RequiredVersion is not null)
                {
                    parameters.Add(Parameters.RequiredVersion, moduleSpecification.RequiredVersion);
                }

                using PowerShell pwsh = PowerShell.Create(this.Runspace);
                _ = pwsh.AddCommand(Commands.InstallModule)
                        .AddParameters(parameters)
                        .AddParameter(Parameters.Force)
                        .InvokeAndStopOnError();
            }
        }

        /// <inheritdoc/>
        public List<Certificate> GetCertsOfValidSignedFiles(string[] paths)
        {
            using PowerShell pwsh = PowerShell.Create(this.Runspace);

            var signatures = pwsh.AddCommand(Commands.GetChildItem)
                                 .AddParameter(Parameters.Path, paths)
                                 .AddCommand(Commands.GetAuthenticodeSignature)
                                 .InvokeAndStopOnError<Signature>();

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
            string oldModulePath = this.GetVariable<string>(Variables.PSModulePath);
            this.SetPSModulePath($"{path};{oldModulePath}");
        }

        /// <inheritdoc/>
        public void PrependPSModulePaths(IReadOnlyList<string> paths)
        {
            string oldModulePath = this.GetVariable<string>(Variables.PSModulePath);
            this.SetPSModulePath($"{string.Join(";", paths)};{oldModulePath}");
        }

        /// <inheritdoc/>
        public void AppendPSModulePath(string path)
        {
            string oldModulePath = this.GetVariable<string>(Variables.PSModulePath);
            this.SetPSModulePath($"{oldModulePath};{path}");
        }

        /// <inheritdoc/>
        public void AppendPSModulePaths(IReadOnlyList<string> paths)
        {
            string oldModulePath = this.GetVariable<string>(Variables.PSModulePath);
            this.SetPSModulePath($"{oldModulePath};{string.Join(";", paths)}");
        }

        /// <inheritdoc/>
        public void CleanupPSModulePath(string path)
        {
            string newModulePath = this.GetVariable<string>(Variables.PSModulePath)
                                       .Replace($"{path};", null)
                                       .Replace($";{path}", null);

            this.SetPSModulePath(newModulePath);
        }

        private bool ValidateModule(ModuleSpecification moduleSpecification)
        {
            var loadedModule = this.GetImportedModule(moduleSpecification);
            if (loadedModule is not null)
            {
                return true;
            }

            var availableModule = this.GetAvailableModule(moduleSpecification);
            if (availableModule is not null)
            {
                this.ImportModule(moduleSpecification);
                return true;
            }

            return false;
        }
    }
}
