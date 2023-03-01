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
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Windows.Security.Cryptography.Certificates;

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
                return this.GetUnitProcessorDetailsLocal(
                    unit.UnitName,
                    dscResourceInfo,
                    detailLevel == ConfigurationUnitDetailLevel.Load);
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

            // Hopefully they will never change the properties name. If someone can explain to me
            // why assign it Name to $_ in Find-DscResource turns into a string in PowerShell but
            // into a PSObject here that would be nice...
            dynamic findResource = getFindResource;
            string findResourceName = findResource.Name.ToString();

            if (detailLevel == ConfigurationUnitDetailLevel.Catalog)
            {
                return new ConfigurationUnitProcessorDetails(
                    findResourceName,
                    null,
                    null,
                    findResource.PSGetModuleInfo,
                    null);
            }

            if (detailLevel == ConfigurationUnitDetailLevel.Download)
            {
                var tempSavePath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
                Directory.CreateDirectory(tempSavePath);
                this.ProcessorEnvironment.SaveModule(getFindResource, tempSavePath);

                var moduleInfo = this.ProcessorEnvironment.GetAvailableModule(
                    Path.Combine(tempSavePath, findResource.PSGetModuleInfo.Name));

                return new ConfigurationUnitProcessorDetails(
                    findResourceName,
                    null,
                    moduleInfo,
                    findResource.PSGetModuleInfo,
                    this.GetCertificates(moduleInfo));
            }

            if (detailLevel == ConfigurationUnitDetailLevel.Load)
            {
                this.ProcessorEnvironment.InstallModule(getFindResource);

                dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);

                if (dscResourceInfo is null)
                {
                    // Well, this is awkward.
                    throw new InstallDscResourceException(
                        unit.UnitName,
                        PowerShellHelpers.CreateModuleSpecification(findResource.ModuleName, findResource.Version));
                }

                return this.GetUnitProcessorDetailsLocal(unit.UnitName, dscResourceInfo, true);
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
                    throw new FindDscResourceNotFoundException(unitInternal.Unit.UnitName);
                }

                this.ProcessorEnvironment.InstallModule(findDscResourceResult);

                // Now we should find it.
                dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);
                if (dscResourceInfo is null)
                {
                    throw new InstallDscResourceException(unitInternal.Unit.UnitName, unitInternal.Module);
                }
            }

            return dscResourceInfo;
        }

        private ConfigurationUnitProcessorDetails GetUnitProcessorDetailsLocal(
            string unitName,
            DscResourceInfoInternal dscResourceInfo,
            bool importModule)
        {
            // I'm looking at you resources under C:\WINDOWS\system32\WindowsPowershell
            if (dscResourceInfo.ModuleName is null ||
                dscResourceInfo.Version is null)
            {
                return new ConfigurationUnitProcessorDetails(
                    dscResourceInfo.Name,
                    dscResourceInfo,
                    null,
                    null,
                    null);
            }

            var module = PowerShellHelpers.CreateModuleSpecification(
                            dscResourceInfo.ModuleName,
                            dscResourceInfo.Version.ToString());

            // Get-InstalledModule only works for modules installed via PowerShell-Get.
            // There are some properties that can only be obtain by that it so is better to take both.
            var moduleInfo = this.ProcessorEnvironment.GetAvailableModule(module);
            var installedModule = this.ProcessorEnvironment.GetInstalledModule(module);

            if (importModule)
            {
                this.ProcessorEnvironment.ImportModule(module);
            }

            return new ConfigurationUnitProcessorDetails(
                dscResourceInfo.Name,
                dscResourceInfo,
                moduleInfo,
                installedModule,
                this.GetCertificates(moduleInfo));
        }

        private List<Certificate>? GetCertificates(PSModuleInfo? moduleInfo)
        {
            if (moduleInfo is null)
            {
                return null;
            }

            // TODO: we still need to investigate more here, but lets start with something.
            var paths = new List<string>();

            var psdPath = Path.Combine(moduleInfo.ModuleBase, $"{moduleInfo.Name}.psd1");
            if (File.Exists(psdPath))
            {
                paths.Add(psdPath);
            }

            var psmPath = Path.Combine(moduleInfo.ModuleBase, $"{moduleInfo.Name}.psm1");
            if (File.Exists(psmPath))
            {
                paths.Add(psmPath);
            }

            return this.ProcessorEnvironment.GetCertsOfValidSignedFiles(paths.ToArray());
        }
    }
}
