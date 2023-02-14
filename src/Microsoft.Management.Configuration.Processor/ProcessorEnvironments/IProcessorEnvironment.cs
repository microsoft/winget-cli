// -----------------------------------------------------------------------------
// <copyright file="IProcessorEnvironment.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.ProcessorEnvironments
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;

    /// <summary>
    /// IProcessorEnvironment. Provides interaction with PowerShell.
    /// </summary>
    internal interface IProcessorEnvironment
    {
        /// <summary>
        /// Gets the runspace.
        /// </summary>
        Runspace Runspace { get; }

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
        DscResourceInfoInternal? GetDscResource(ConfigurationUnitInternal unitInternal);

        /// <summary>
        /// Calls Invoke-DscResource -Method Get from this module. Maybe change input to configuration unit internal.
        /// </summary>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>Properties of resource.</returns>
        ValueSet InvokeGetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Invoke-DscResource -Method Test from this module. Maybe change input to configuration unit internal.
        /// </summary>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>Is in desired state.</returns>
        bool InvokeTestResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification);

        /// <summary>
        /// Calls Invoke-DscResource -Method Set from this module. Maybe change input to configuration unit internal.
        /// </summary>
        /// <param name="settings">Settings.</param>
        /// <param name="name">Name.</param>
        /// <param name="moduleSpecification">Module specification.</param>
        /// <returns>If a reboot is required.</returns>
        bool InvokeSetResource(ValueSet settings, string name, ModuleSpecification? moduleSpecification);

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
    }
}
