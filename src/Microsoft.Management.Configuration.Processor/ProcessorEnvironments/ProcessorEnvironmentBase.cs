// -----------------------------------------------------------------------------
// <copyright file="ProcessorEnvironmentBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Runspaces
{
    using System.Collections.Generic;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// Base class for processor environments.
    /// </summary>
    internal abstract class ProcessorEnvironmentBase : IProcessorEnvironment
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessorEnvironmentBase"/> class.
        /// </summary>
        /// <param name="runspace">PowerShell Runspace.</param>
        /// <param name="dscModule">IDscModule.</param>
        public ProcessorEnvironmentBase(Runspace runspace, IDscModule dscModule)
        {
            this.Runspace = runspace;
            this.DscModule = dscModule;
        }

        /// <inheritdoc/>
        public Runspace Runspace { get; }

        /// <summary>
        /// Gets the DscModule.
        /// </summary>
        protected IDscModule DscModule { get; }

        /// <inheritdoc/>
        public abstract void ValidateRunspace();

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
    }
}
