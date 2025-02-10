// -----------------------------------------------------------------------------
// <copyright file="IProcessorEnvironment.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments
{
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;
    using Windows.Security.Cryptography.Certificates;

    /// <summary>
    /// IProcessorEnvironment. Provides interaction with PowerShell.
    /// </summary>
    internal interface IProcessorEnvironment
    {
        /// <summary>
        /// Gets the runspace.
        /// </summary>
        System.Management.Automation.Runspaces.Runspace Runspace { get; }

        /// <summary>
        /// Validates the runspace.
        /// </summary>
        void ValidateRunspace();

        /// <summary>
        /// Gets all DSC resource.
        /// </summary>
        /// <returns>A list with the DSC resource.</returns>
        IReadOnlyList<DscResourceInfoInternal> GetAllDscResources();

        /// <summary>
        /// Gets all resources in a module.
        /// </summary>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>List of resources.</returns>
        IReadOnlyList<DscResourceInfoInternal> GetDscResourcesInModule(ModuleSpecification moduleSpecification);

        /// <summary>
        /// Gets a DSC Resource.
        /// </summary>
        /// <param name="unitInternal">Configuration unit internal.</param>
        /// <returns>DSC Resource.</returns>
        DscResourceInfoInternal? GetDscResource(ConfigurationUnitAndModule unitInternal);

        /// <summary>
        /// Calls Invoke-DscResource -Method Get from this module.
        /// </summary>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>Properties of resource.</returns>
        ValueSet InvokeGetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Invoke-DscResource -Method Test from this module.
        /// </summary>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>Is in desired state.</returns>
        bool InvokeTestResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Invoke-DscResource -Method Set from this module.
        /// </summary>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>If a reboot is required.</returns>
        bool InvokeSetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Get-Module with fully qualified name.
        /// </summary>
        /// <param name="moduleSpecification">Module name.</param>
        /// <returns>PSModuleInfo, null if not imported.</returns>
        PSModuleInfo? GetImportedModule(ModuleSpecification moduleSpecification);

        /// <summary>
        /// Calls Get-Module with the fully qualified name and using ListAvailable.
        /// </summary>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>PSModuleInfo, null if not found.</returns>
        PSModuleInfo? GetAvailableModule(ModuleSpecification moduleSpecification);

        /// <summary>
        /// Calls Get-Module from a path using ListAvailable.
        /// </summary>
        /// <param name="path">Path.</param>
        /// <returns>The first module returned, null if none.</returns>
        PSModuleInfo? GetAvailableModule(string path);

        /// <summary>
        /// Calls Import-Module with the fully qualified name.
        /// </summary>
        /// <param name="moduleSpecification">Module specification.</param>
        void ImportModule(ModuleSpecification moduleSpecification);

        /// <summary>
        /// Calls Import-Module with a file path.
        /// </summary>
        /// <param name="path">Module file path.</param>
        void ImportModule(string path);

        /// <summary>
        /// Calls Get-InstalledModule.
        /// </summary>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>Module info, null if not installed.</returns>
        PSObject? GetInstalledModule(ModuleSpecification moduleSpecification);

        /// <summary>
        /// Calls Find-Module.
        /// </summary>
        /// <param name="unitInternal">Configuration unit internal.</param>
        /// <returns>Module info, null if not found.</returns>
        PSObject? FindModule(ConfigurationUnitInternal unitInternal);

        /// <summary>
        /// Calls Find-DscResource.
        /// </summary>
        /// <param name="unitInternal">Configuration unit internal.</param>
        /// <returns>Dsc Resource info, null if not found.</returns>
        PSObject? FindDscResource(ConfigurationUnitInternal unitInternal);

        /// <summary>
        /// Calls Save-Module -InputObject object -Path location.
        /// Input object must be the result of Find cmdlets of PowerShellGet.
        /// </summary>
        /// <param name="inputObject">Input object.</param>
        /// <param name="location">Location to save module.</param>
        void SaveModule(PSObject inputObject, string location);

        /// <summary>
        /// Calls Save-Module.
        /// </summary>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <param name="location">Location to save module.</param>
        void SaveModule(ModuleSpecification moduleSpecification, string location);

        /// <summary>
        /// Calls Install-Module -InputObject object.
        /// Input object must be the result of Find cmdlets of PowerShellGet.
        /// </summary>
        /// <param name="inputObject">Input object.</param>
        void InstallModule(PSObject inputObject);

        /// <summary>
        /// Calls Install-Module with a module specification.
        /// </summary>
        /// <param name="moduleSpecification">Module specification.</param>
        void InstallModule(ModuleSpecification moduleSpecification);

        /// <summary>
        /// Get unique certificates of valid signed files from the specified paths.
        /// </summary>
        /// <param name="paths">Path.</param>
        /// <returns>List with valid signatures.</returns>
        List<Certificate> GetCertsOfValidSignedFiles(string[] paths);

        /// <summary>
        /// Gets the value of a variable.
        /// </summary>
        /// <typeparam name="TType">Type of the variable.</typeparam>
        /// <param name="name">Name of variable.</param>
        /// <returns>The value of a variable, null if doesn't exist.</returns>
        TType GetVariable<TType>(string name);

        /// <summary>
        /// Sets a variable with its value.
        /// </summary>
        /// <param name="name">Name of variable.</param>
        /// <param name="value">Value of variable.</param>
        void SetVariable(string name, object value);

        /// <summary>
        /// Overwrites PSModulePath with the specified path.
        /// </summary>
        /// <param name="path">Path.</param>
        void SetPSModulePath(string path);

        /// <summary>
        /// Overwrites PSModulePath with the specified paths.
        /// </summary>
        /// <param name="paths">Paths.</param>
        void SetPSModulePaths(IReadOnlyList<string> paths);

        /// <summary>
        /// Prepends path to the PSModulePath.
        /// </summary>
        /// <param name="path">Path.</param>
        void PrependPSModulePath(string path);

        /// <summary>
        /// Prepends paths to the PSModulePath.
        /// </summary>
        /// <param name="paths">Paths.</param>
        void PrependPSModulePaths(IReadOnlyList<string> paths);

        /// <summary>
        /// Append path to the PSModulePath.
        /// </summary>
        /// <param name="path">Path.</param>
        void AppendPSModulePath(string path);

        /// <summary>
        /// Append paths to the PSModulePath.
        /// </summary>
        /// <param name="paths">Path.</param>
        void AppendPSModulePaths(IReadOnlyList<string> paths);

        /// <summary>
        /// Removes a path from the module path.
        /// </summary>
        /// <param name="path">Path.</param>
        void CleanupPSModulePath(string path);

        /// <summary>
        /// Sets the location for installing modules.
        /// </summary>
        /// <param name="location">Location.</param>
        /// <param name="customLocation">Path for custom location.</param>
        void SetLocation(PowerShellConfigurationProcessorLocation location, string? customLocation);
    }
}
