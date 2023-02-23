﻿// -----------------------------------------------------------------------------
// <copyright file="DscModuleV2.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DscModule
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Management.Automation;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// PSDesiredStateConfiguration.
    /// </summary>
    internal class DscModuleV2 : IDscModule
    {
        private const string ExperimentalFeatureName = "PSDesiredStateConfiguration.InvokeDscResource";
        private const string InDesiredState = "InDesiredState";
        private const string RebootRequired = "RebootRequired";

        /// <summary>
        /// Initializes a new instance of the <see cref="DscModuleV2"/> class.
        /// </summary>
        public DscModuleV2()
        {
        }

        /// <inheritdoc/>
        public string ModuleName { get; } = Modules.PSDesiredStateConfiguration;

        /// <inheritdoc/>
        public string GetDscResourceCmd { get; } = Commands.GetDscResource;

        /// <inheritdoc/>
        public string InvokeDscResourceCmd { get; } = Commands.InvokeDscResource;

        /// <inheritdoc/>
        public void ValidateModule(Runspace runspace)
        {
            // TODO: Validate min version.
        }

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetAllDscResources(Runspace runspace)
        {
            var result = new List<DscResourceInfoInternal>();

            using PowerShell pwsh = PowerShell.Create(runspace);
            var resources = pwsh.AddCommand(this.GetDscResourceCmd)
                                .InvokeAndStopOnError();

            return this.ConvertToDscResourceInfoInternal(resources);
        }

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetDscResourcesInModule(
            Runspace runspace,
            ModuleSpecification moduleSpecification)
        {
            var result = new List<DscResourceInfoInternal>();

            using PowerShell pwsh = PowerShell.Create(runspace);

            var resources = pwsh.AddCommand(this.GetDscResourceCmd)
                                .AddParameter(Parameters.Module, moduleSpecification)
                                .InvokeAndStopOnError();

            return this.ConvertToDscResourceInfoInternal(resources);
        }

        /// <inheritdoc/>
        public DscResourceInfoInternal? GetDscResource(
            Runspace runspace,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(runspace);
            pwsh.AddCommand(this.GetDscResourceCmd)
                .AddParameter(Parameters.Name, name);

            if (moduleSpecification is not null)
            {
                pwsh.AddParameter(Parameters.Module, moduleSpecification);
            }

            var resources = pwsh.InvokeAndStopOnError();

            var dscResourceInfos = this.ConvertToDscResourceInfoInternal(resources);

            if (dscResourceInfos.Count == 0)
            {
                return null;
            }
            else if (dscResourceInfos.Count > 1)
            {
                throw new ArgumentException(name);
            }

            return dscResourceInfos[0];
        }

        /// <inheritdoc/>
        public ValueSet InvokeGetResource(
            Runspace runspace,
            ValueSet settings,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(runspace);

            var getResult = pwsh.AddCommand(this.InvokeDscResourceCmd)
                                .AddParameters(PrepareInvokeParameters(name, settings, moduleSpecification))
                                .AddParameter(Parameters.Method, DscMethods.Get)
                                .InvokeAndStopOnError()
                                .FirstOrDefault();

            if (getResult is null)
            {
                throw new ArgumentException(name);
            }

            // Script based resource.
            if (getResult.BaseObject is Hashtable)
            {
                Hashtable hashTable = (Hashtable)getResult.BaseObject;
                var resultSettings = new ValueSet();
                foreach (DictionaryEntry entry in hashTable)
                {
                    resultSettings.Add(entry.Key as string, entry.Value);
                }

                return resultSettings;
            }

            // Class-based resource.
            return TypeHelpers.GetAllPropertiesValues(getResult.BaseObject);
        }

        /// <inheritdoc/>
        public bool InvokeTestResource(
            Runspace runspace,
            ValueSet settings,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(runspace);

            // Returned type is InvokeDscResourceTestResult which is a PowerShell classed defined
            // in PSDesiredStateConfiguration.psm1.
            dynamic? testResult = pwsh.AddCommand(this.InvokeDscResourceCmd)
                                      .AddParameters(PrepareInvokeParameters(name, settings, moduleSpecification))
                                      .AddParameter(Parameters.Method, DscMethods.Test)
                                      .InvokeAndStopOnError()
                                      .FirstOrDefault();

            if (testResult is null ||
                !TypeHelpers.PropertyWithTypeExists<bool>(testResult, InDesiredState))
            {
                throw new ArgumentException(name);
            }

            return testResult?.InDesiredState;
        }

        /// <inheritdoc/>
        public bool InvokeSetResource(
            Runspace runspace,
            ValueSet settings,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            using PowerShell pwsh = PowerShell.Create(runspace);

            // Returned type is InvokeDscResourceSetResult which is a PowerShell classed defined
            // in PSDesiredStateConfiguration.psm1.
            dynamic? setResult = pwsh.AddCommand(this.InvokeDscResourceCmd)
                                     .AddParameters(PrepareInvokeParameters(name, settings, moduleSpecification))
                                     .AddParameter(Parameters.Method, DscMethods.Set)
                                     .InvokeAndStopOnError()
                                     .FirstOrDefault();

            if (setResult is null ||
                !TypeHelpers.PropertyWithTypeExists<bool>(setResult, RebootRequired))
            {
                throw new ArgumentException(name);
            }

            return setResult?.RebootRequired;
        }

        private static Dictionary<string, object> PrepareInvokeParameters(
            string name,
            ValueSet settings,
            ModuleSpecification? moduleSpecification)
        {
            var properties = new Hashtable();
            foreach (var setting in settings)
            {
                properties.Add(setting.Key, setting.Value);
            }

            var parameters = new Dictionary<string, object>()
            {
                { Parameters.Name, name },
                { Parameters.Property, properties },
            };

            if (moduleSpecification is not null)
            {
                parameters.Add(Parameters.ModuleName, moduleSpecification);
            }

            return parameters;
        }

        private List<DscResourceInfoInternal> ConvertToDscResourceInfoInternal(Collection<PSObject> psObjects)
        {
            var result = new List<DscResourceInfoInternal>();
            foreach (dynamic psObject in psObjects)
            {
                var dscResourceInfo = new DscResourceInfoInternal(psObject);

                // Explicitly don't support old DSC resources from v1 PSDesiredStateConfiguration.
                // Even if the windows sytem32 windows powershell module path is removed they
                // will show up.
                if (dscResourceInfo.ParentPath!.StartsWith(
                        @"C:\WINDOWS\system32\WindowsPowershell\v1.0\Modules\PsDesiredStateConfiguration\DscResources",
                        StringComparison.OrdinalIgnoreCase) ||
                    dscResourceInfo.ParentPath!.StartsWith(
                        @"C:\Program Files\WindowsPowerShell\Modules\PackageManagement\1.0.0.1",
                        StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                result.Add(dscResourceInfo);
            }

            return result;
        }
    }
}
