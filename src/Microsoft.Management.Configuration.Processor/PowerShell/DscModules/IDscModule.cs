// -----------------------------------------------------------------------------
// <copyright file="IDscModule.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.DscModules
{
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;

    /// <summary>
    /// Interface for defining a PSDesiredStateConfiguration module.
    /// </summary>
    internal interface IDscModule
    {
        /// <summary>
        /// Gets the module specification.
        /// </summary>
        ModuleSpecification ModuleSpecification { get; }

        /// <summary>
        /// Gets the name of the Get-DscResource Cmdlet.
        /// </summary>
        string GetDscResourceCmd { get; }

        /// <summary>
        /// Gets the name of the Invoke-DscResource Cmdlet.
        /// </summary>
        string InvokeDscResourceCmd { get; }

        /// <summary>
        /// Gets all DSC resource.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <returns>A list with the DSC resource.</returns>
        IReadOnlyList<DscResourceInfoInternal> GetAllDscResources(PowerShell pwsh);

        /// <summary>
        /// Gets all resources in a module.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>List of resources of that module and version.</returns>
        IReadOnlyList<DscResourceInfoInternal> GetDscResourcesInModule(PowerShell pwsh, ModuleSpecification moduleSpecification);

        /// <summary>
        /// Gets a DSC Resource.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>DSC Resource from that module and version.</returns>
        DscResourceInfoInternal? GetDscResource(PowerShell pwsh, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Invoke-DscResource -Method Get from this module.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>Properties of resource.</returns>
        ValueSet InvokeGetResource(PowerShell pwsh, ValueSet settings, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Invoke-DscResource -Method Test from this module.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>Is in desired state.</returns>
        bool InvokeTestResource(PowerShell pwsh, ValueSet settings, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Invoke-DscResource -Method Set from this module.
        /// </summary>
        /// <param name="pwsh">PowerShell.</param>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>If a reboot is required.</returns>
        bool InvokeSetResource(PowerShell pwsh, ValueSet settings, string name, ModuleSpecification? moduleSpecification);
    }
}
