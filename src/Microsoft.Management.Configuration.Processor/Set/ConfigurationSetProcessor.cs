// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProcessor.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Set
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Unit;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

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
        public IConfigurationUnitProcessorDetails GetUnitProcessorDetails(
            ConfigurationUnit unit,
            ConfigurationUnitDetailLevel detailLevel)
        {
            // Local
            // Call Get-DscResource
            // If resource
            //   Call Get-InstalledModule
            //   Construct
            // else
            //   return null or throw.

            // Catalog
            // Call Get-DscResource
            // if resource
            //    Call Get-InstalledModule
            //    Construct full
            // else
            //    Call Find-DscResource
            //    Construct no dsc resources.

            // Download
            // Call Get-DscResource
            // if resource
            //    Call Get-InstalledModule
            //    Construct full
            // else
            //    Call Find-DscResource
            //    Call Save-Module
            //    Construct no dsc resources.

            // Load
            // Call Get-DscResource
            // if resource
            //    Call Get-InstalledModule
            //    Construct full
            // else
            //    Call Find-DscResource
            //    Call Install-Module.
            //    Call Load-Module.
            //    Call Gets-DscResource.
            //    Construct
            throw new NotImplementedException();
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
    }
}
