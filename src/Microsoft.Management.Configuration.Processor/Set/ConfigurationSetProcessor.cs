﻿// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Set
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// Configuration set processor.
    /// </summary>
    internal sealed class ConfigurationSetProcessor : IConfigurationSetProcessor
    {
        private readonly ConfigurationSet configurationSet;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProcessor"/> class.
        /// </summary>
        /// <param name="processorEnvironment">The processor environment.</param>
        /// <param name="configurationSet">Configuration set.</param>
        public ConfigurationSetProcessor(IProcessorEnvironment processorEnvironment, ConfigurationSet configurationSet)
        {
            this.ProcessorEnvironment = processorEnvironment;
            this.configurationSet = configurationSet;
        }

        /// <summary>
        /// Gets the processor environment.
        /// </summary>
        internal IProcessorEnvironment ProcessorEnvironment { get; }

        /// <summary>
        /// Creates a configuration unit processor for the given unit.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        /// <param name="directivesOverlay">Allows for the ConfigurationProcessor to alter behavior without needing to change the unit itself.</param>
        /// <returns>A configuration unit processor.</returns>
        public IConfigurationUnitProcessor CreateUnitProcessor(
            ConfigurationUnit unit,
            IReadOnlyDictionary<string, object>? directivesOverlay)
        {
            var configurationUnitInternal = new ConfigurationUnitInternal(unit, directivesOverlay);

            var dscResourceInfo = this.PrepareUnitForProcessing(configurationUnitInternal);

            return new ConfigurationUnitProcessor(
                this.ProcessorEnvironment,
                new ConfigurationUnitAndResource(configurationUnitInternal, dscResourceInfo));
        }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="unit">Configuration unit.</param>
        /// <param name="detailLevel">Detail level.</param>
        /// <returns>Configuration unit processor details.</returns>
        public IConfigurationUnitProcessorDetails? GetUnitProcessorDetails(
            ConfigurationUnit unit,
            ConfigurationUnitDetailLevel detailLevel)
        {
            var unitInternal = new ConfigurationUnitInternal(unit);
            var dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);

            if (dscResourceInfo is not null)
            {
                return this.GetUnitProcessorDetailsLocal(unit.UnitName, dscResourceInfo);
            }

            if (detailLevel == ConfigurationUnitDetailLevel.Local)
            {
                // Not found locally.
                return null;
            }

            var getFindResource = this.ProcessorEnvironment.FindDscResource(unitInternal);
            if (getFindResource is null)
            {
                // Not found in catalog.
                return null;
            }

            // Hopefully they will never have the property name.
            dynamic findResource = getFindResource as dynamic;

            if (detailLevel == ConfigurationUnitDetailLevel.Catalog)
            {
                return new ConfigurationUnitProcessorDetails(
                    findResource.Name,
                    null,
                    null,
                    findResource.PSGetModuleInfo);
            }

            if (detailLevel == ConfigurationUnitDetailLevel.Download)
            {
                var tempSavePath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
                Directory.CreateDirectory(tempSavePath);
                this.ProcessorEnvironment.SaveModule(getFindResource, tempSavePath);

                var moduleInfo = this.ProcessorEnvironment.GetModule(
                    Path.Combine(tempSavePath, findResource.PSGetModuleInfo.Name));

                // TODO: Send path to get cert signing info and delete directory.
                // We can't get the resources information because Get-DscResource just look
                // in the PSModulePaths.
                return new ConfigurationUnitProcessorDetails(
                    findResource.Name,
                    null,
                    moduleInfo,
                    findResource.PSGetModuleInfo);
            }

            if (detailLevel == ConfigurationUnitDetailLevel.Load)
            {
                this.ProcessorEnvironment.InstallModule(getFindResource);

                dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);

                if (dscResourceInfo is null)
                {
                    // Well, this is awkward.
                    return null;
                }

                return this.GetUnitProcessorDetailsLocal(unit.UnitName, dscResourceInfo);
            }

            return null;
        }

        private DscResourceInfoInternal PrepareUnitForProcessing(ConfigurationUnitInternal unitInternal)
        {
            // Invoke-DscResource makes a call to Get-DscResource which looks at the entire PSModulePath
            // to see if a resource exists. DscResourcesMap is an attempt to try to optimize Get-DscResource
            // by making just one call and get all of them, but it doesn't support minVersion and maxVersion.
            // For now, lets make PowerShell fully figure out which module to use and try to optimize it later.
            // This class will have a private member Lazy<DscResourcesMap> which will be initialized by calling
            // this.ProcessorEnvironment.GetAllDscResources()
            // To improve the performance even more, we will still need Invoke-DscResource to be update to
            // get a DSC resource info object instead of calling Get-DscResource every time.
            var dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);

            if (dscResourceInfo is null)
            {
                var findDscResourceResult = this.ProcessorEnvironment.FindDscResource(unitInternal);

                if (findDscResourceResult is null)
                {
                    string message = $"Resource = {unitInternal.Unit.UnitName}";
                    throw new FindDscResourceNotFoundException(message);
                }

                // TODO: hook up policies and enable this and save the module first and look for files, per set.
                // string savePath = "TODO: Create path";
                // this.ProcessorEnvironment.SaveModule(findDscResourceResult, savePath);
                // string savedModulePath = Path.Combine(
                //     savePath,
                //     (string)findDscResourceResult.Properties[Parameters.ModuleName].Value);
                // string[] paths = new string[]
                // {
                //     $"{savedModulePath}\\*.dll",
                //     $"{savedModulePath}\\*.psd1",
                //     $"{savedModulePath}\\*.psm1",
                // };
                // this.ProcessorEnvironment.VerifySignature(paths);

                // Install module for now, when the validation module gets fully implemented
                // we can improve the performance by moving the modules file somewhere in the
                // PSModulePath directory. We need to either Install-Module or move it for DSC cmdlets
                // to find the resources as the quarantine path will not be in the PSModulePath of
                // this runspace.
                this.ProcessorEnvironment.InstallModule(findDscResourceResult);

                // Now we should find it.
                dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);
                if (dscResourceInfo is null)
                {
                    string message = $"Resource = {unitInternal.Unit.UnitName} Module {unitInternal.Module}";
                    throw new GetDscResourceNotFoundException(message);
                }
            }

            return dscResourceInfo;
        }

        private ConfigurationUnitProcessorDetails GetUnitProcessorDetailsLocal(string unitName, DscResourceInfoInternal dscResourceInfo)
        {
            // I'm looking at you resources under C:\WINDOWS\system32\WindowsPowershell
            if (dscResourceInfo.ModuleName is null ||
                dscResourceInfo.Version is null)
            {
                return new ConfigurationUnitProcessorDetails(
                    dscResourceInfo.Name,
                    dscResourceInfo,
                    null,
                    null);
            }

            var module = PowerShellHelpers.CreateModuleSpecification(
                            dscResourceInfo.ModuleName,
                            dscResourceInfo.Version.ToString());

            // Get-InstalledModule only works for modules installed via PowerShell-Get.
            // There are some properties that can only be obtain by that it so is better to take both.
            var moduleInfo = this.ProcessorEnvironment.GetModule(module);
            var installedModule = this.ProcessorEnvironment.GetInstalledModule(module);

            return new ConfigurationUnitProcessorDetails(
                dscResourceInfo.Name,
                dscResourceInfo,
                moduleInfo,
                installedModule);
        }
    }
}
