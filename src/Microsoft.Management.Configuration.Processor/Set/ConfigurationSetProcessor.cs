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
    using System.Runtime.CompilerServices;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Windows.Security.Cryptography.Certificates;

    /// <summary>
    /// Configuration set processor.
    /// </summary>
    internal sealed class ConfigurationSetProcessor : IConfigurationSetProcessor
    {
        private readonly ConfigurationSet? configurationSet;
        private List<ConfigurationUnit> limitUnitList = new List<ConfigurationUnit>();

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProcessor"/> class.
        /// </summary>
        /// <param name="processorEnvironment">The processor environment.</param>
        /// <param name="configurationSet">Configuration set.</param>
        /// <param name="isLimitMode">Whether the set processor should work in limitation mode.</param>
        public ConfigurationSetProcessor(IProcessorEnvironment processorEnvironment, ConfigurationSet? configurationSet, bool isLimitMode = false)
        {
            this.ProcessorEnvironment = processorEnvironment;
            this.configurationSet = configurationSet;
            this.IsLimitMode = isLimitMode;

            // In limit mode, configurationSet is the limitation set to be used. It cannot be null.
            if (this.IsLimitMode)
            {
                if (this.configurationSet == null)
                {
                    throw new ArgumentNullException(nameof(configurationSet), "configurationSet is required in limit mode.");
                }

                foreach (var unit in this.configurationSet.Units)
                {
                    this.limitUnitList.Add(unit);
                }
            }
        }

        /// <summary>
        /// Gets or initializes the set processor factory.
        /// </summary>
        internal PowerShellConfigurationSetProcessorFactory? SetProcessorFactory { get; init; }

        /// <summary>
        /// Gets the processor environment.
        /// </summary>
        internal IProcessorEnvironment ProcessorEnvironment { get; }

        /// <summary>
        /// Gets a value indicating whether the set processor is running in limit mode.
        /// </summary>
        internal bool IsLimitMode { get; private set; }

        /// <summary>
        /// Creates a configuration unit processor for the given unit.
        /// </summary>
        /// <param name="incomingUnit">Configuration unit.</param>
        /// <returns>A configuration unit processor.</returns>
        public IConfigurationUnitProcessor CreateUnitProcessor(
            ConfigurationUnit incomingUnit)
        {
            try
            {
                this.OnDiagnostics(DiagnosticLevel.Informational, $"GetUnitProcessorDetails is running in limit mode: {this.IsLimitMode}.");

                // CreateUnitProcessor can only be called once on each configuration unit in limit mode.
                var unit = this.GetConfigurationUnit(incomingUnit, true);

                var configurationUnitInternal = new ConfigurationUnitInternal(unit, this.configurationSet?.Path) { UnitTypeIsResourceName = IsUnitTypeResourceName(this.configurationSet?.SchemaVersion) };
                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Creating unit processor for: {configurationUnitInternal.QualifiedName}...");

                var dscResourceInfo = this.PrepareUnitForProcessing(configurationUnitInternal);

                this.OnDiagnostics(DiagnosticLevel.Verbose, "... done creating unit processor.");
                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Using unit from location: {dscResourceInfo.Path}");
                return new ConfigurationUnitProcessor(
                    this.ProcessorEnvironment,
                    new ConfigurationUnitAndResource(configurationUnitInternal, dscResourceInfo),
                    this.IsLimitMode)
                { SetProcessorFactory = this.SetProcessorFactory };
            }
            catch (Exception ex)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, ex.ToString());
                throw;
            }
        }

        /// <summary>
        /// Gets the configuration unit processor details for the given unit.
        /// </summary>
        /// <param name="incomingUnit">Configuration unit.</param>
        /// <param name="detailFlags">Detail flags.</param>
        /// <returns>Configuration unit processor details.</returns>
        public IConfigurationUnitProcessorDetails? GetUnitProcessorDetails(
            ConfigurationUnit incomingUnit,
            ConfigurationUnitDetailFlags detailFlags)
        {
            try
            {
                this.OnDiagnostics(DiagnosticLevel.Informational, $"GetUnitProcessorDetails is running in limit mode: {this.IsLimitMode}.");

                // GetUnitProcessorDetails can be invoked multiple times on each configuration unit in limit mode.
                var unit = this.GetConfigurationUnit(incomingUnit);

                var unitInternal = new ConfigurationUnitInternal(unit, this.configurationSet?.Path);
                this.OnDiagnostics(DiagnosticLevel.Verbose, $"Getting unit details [{detailFlags}] for: {unitInternal.QualifiedName}");

                // (Local | Download | Load) will all work off of local files, so if any one is an option just use the local module info if found.
                DscResourceInfoInternal? dscResourceInfo = null;
                if (detailFlags.HasFlag(ConfigurationUnitDetailFlags.Local) || detailFlags.HasFlag(ConfigurationUnitDetailFlags.Download) || detailFlags.HasFlag(ConfigurationUnitDetailFlags.Load))
                {
                    dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);
                }

                if (dscResourceInfo is not null)
                {
                    return this.GetUnitProcessorDetailsLocal(
                        dscResourceInfo.Name,
                        dscResourceInfo,
                        detailFlags.HasFlag(ConfigurationUnitDetailFlags.Load));
                }

                if (!(detailFlags.HasFlag(ConfigurationUnitDetailFlags.Catalog) || detailFlags.HasFlag(ConfigurationUnitDetailFlags.Download) || detailFlags.HasFlag(ConfigurationUnitDetailFlags.Load)))
                {
                    // Not found locally.
                    return null;
                }

                var unitModuleInfo = this.FindUnitModule(unitInternal);
                if (unitModuleInfo is null)
                {
                    // Not found in catalog.
                    return null;
                }

                PSObject foundModule = unitModuleInfo.Value.Module;
                string resourceName = unitModuleInfo.Value.ResourceName;

                dynamic foundModuleInfo = foundModule;

                if (detailFlags.HasFlag(ConfigurationUnitDetailFlags.Catalog))
                {
                    return new ConfigurationUnitProcessorDetails(
                        resourceName,
                        null,
                        null,
                        foundModule,
                        null);
                }

                if (detailFlags.HasFlag(ConfigurationUnitDetailFlags.Download))
                {
                    var tempSavePath = Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString());
                    Directory.CreateDirectory(tempSavePath);
                    this.ProcessorEnvironment.SaveModule(foundModule, tempSavePath);

                    var moduleInfo = this.ProcessorEnvironment.GetAvailableModule(
                        Path.Combine(tempSavePath, foundModuleInfo.Name));

                    return new ConfigurationUnitProcessorDetails(
                        resourceName,
                        null,
                        moduleInfo,
                        foundModule,
                        this.GetCertificates(moduleInfo));
                }

                if (detailFlags.HasFlag(ConfigurationUnitDetailFlags.Load))
                {
                    this.ProcessorEnvironment.InstallModule(foundModule);

                    dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);

                    if (dscResourceInfo is null)
                    {
                        // Well, this is awkward.
                        throw new InstallDscResourceException(
                            unitInternal.ResourceName,
                            PowerShellHelpers.CreateModuleSpecification(foundModuleInfo.Name, foundModuleInfo.Version));
                    }

                    return this.GetUnitProcessorDetailsLocal(dscResourceInfo.Name, dscResourceInfo, true);
                }

                return null;
            }
            catch (Exception ex)
            {
                this.OnDiagnostics(DiagnosticLevel.Error, ex.ToString());
                throw;
            }
        }

        private static bool IsUnitTypeResourceName(string? schemaVersion)
        {
            return schemaVersion != null && schemaVersion == "0.1";
        }

        private static bool ConfigurationUnitEquals(ConfigurationUnit first, ConfigurationUnit second)
        {
            if (first.Identifier != second.Identifier ||
                first.Type != second.Type ||
                first.Intent != second.Intent)
            {
                return false;
            }

            if (!first.Settings.ContentEquals(second.Settings))
            {
                return false;
            }

            if (!first.Metadata.ContentEquals(second.Metadata))
            {
                return false;
            }

            // Note: Consider group units logic when group units are supported.
            return true;
        }

        /// <summary>
        /// Finds the module and preferred resource name for processing the configuration unit.
        /// </summary>
        /// <param name="unitInternal">The internal configuration unit.</param>
        /// <returns>A tuple containing the module info and preferred resource name, or null if not found.</returns>
        private (PSObject Module, string ResourceName)? FindUnitModule(ConfigurationUnitInternal unitInternal)
        {
            PSObject? foundModule = null;
            string resourceName = string.Empty;

            // If module has been specified, find it and assume that the resource will be within it.
            // Do this first as we do not currently gain much from FindDscResource; if that changes then it can be the primary.
            if (unitInternal.Module != null)
            {
                foundModule = this.ProcessorEnvironment.FindModule(unitInternal);
                if (foundModule != null)
                {
                    resourceName = unitInternal.ResourceName;
                }
            }
            else
            {
                dynamic? foundResource = this.ProcessorEnvironment.FindDscResource(unitInternal);
                if (foundResource != null)
                {
                    foundModule = foundResource.PSGetModuleInfo;

                    // Hopefully they will never change the properties name. If someone can explain to me
                    // why assign it Name to $_ in Find-DscResource turns into a string in PowerShell but
                    // into a PSObject here that would be nice...
                    resourceName = foundResource.Name.ToString();
                }
            }

            if (foundModule != null)
            {
                return (foundModule, resourceName);
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
                var findUnitModuleResult = this.FindUnitModule(unitInternal);

                if (findUnitModuleResult is null)
                {
                    throw new FindDscResourceNotFoundException(unitInternal.ResourceName, unitInternal.Module);
                }

                this.ProcessorEnvironment.InstallModule(findUnitModuleResult.Value.Module);

                // Now we should find it.
                dscResourceInfo = this.ProcessorEnvironment.GetDscResource(unitInternal);
                if (dscResourceInfo is null)
                {
                    throw new InstallDscResourceException(unitInternal.ResourceName, unitInternal.Module);
                }
            }

            // PowerShell will prompt the user when a module that is downloaded from the internet is imported.
            // For a hosted environment, this will throw an exception because it doesn't support user interaction.
            // In the case we don't import the module here, eventually Invoke-DscResource will fail for class
            // resources because they will call a method on a null obj. It is easier to just fail here.
            // The exception being thrown will have the correct details (user needs to call Unblock-File)
            // instead of the cryptic Invoke with 0 arguments.
            if (!string.IsNullOrEmpty(dscResourceInfo.Path))
            {
                try
                {
                    this.ProcessorEnvironment.ImportModule(dscResourceInfo.Path);
                }
                catch (Exception e)
                {
                    throw new ImportModuleException(dscResourceInfo.ModuleName, e);
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

        private void OnDiagnostics(DiagnosticLevel level, string message)
        {
            this.SetProcessorFactory?.OnDiagnostics(level, message);
        }

        [MethodImpl(MethodImplOptions.Synchronized)]
        private ConfigurationUnit GetConfigurationUnit(ConfigurationUnit incomingUnit, bool useLimitList = false)
        {
            if (this.IsLimitMode)
            {
                if (this.configurationSet == null)
                {
                    throw new InvalidOperationException("Configuration set should not be null in limit mode.");
                }

                var unitList = useLimitList ? this.limitUnitList : this.configurationSet.Units;

                for (int i = 0; i < unitList.Count; i++)
                {
                    var unit = unitList[i];
                    if (ConfigurationUnitEquals(incomingUnit, unit))
                    {
                        if (useLimitList)
                        {
                            this.limitUnitList.RemoveAt(i);
                        }

                        return unit;
                    }

                    // Note: Consider group units logic when group units are supported.
                }

                this.OnDiagnostics(DiagnosticLevel.Error, "Configuration unit not found in limit mode.");
                throw new InvalidOperationException("Configuration unit not found in limit mode.");
            }
            else
            {
                return incomingUnit;
            }
        }
    }
}
