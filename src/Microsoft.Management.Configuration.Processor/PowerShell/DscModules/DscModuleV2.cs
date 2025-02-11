// -----------------------------------------------------------------------------
// <copyright file="DscModuleV2.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.DscModules
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.PowerShell.Extensions;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;
    using static Microsoft.Management.Configuration.Processor.PowerShell.Constants.PowerShellConstants;

    /// <summary>
    /// PSDesiredStateConfiguration v2.
    /// </summary>
    internal class DscModuleV2 : IDscModule
    {
        private const string InDesiredState = "InDesiredState";
        private const string RebootRequired = "RebootRequired";

        private static readonly IEnumerable<string> ExclusionResourcesParentPath = new string[]
        {
            @"C:\WINDOWS\system32\WindowsPowershell\v1.0\Modules\PsDesiredStateConfiguration\DscResources",
            @"C:\Program Files\WindowsPowerShell\Modules\PackageManagement\1.0.0.1",
        };

        /// <summary>
        /// Initializes a new instance of the <see cref="DscModuleV2"/> class.
        /// </summary>
        public DscModuleV2()
        {
            this.ModuleSpecification = PowerShellHelpers.CreateModuleSpecification(
                Modules.PSDesiredStateConfiguration,
                minVersion: Modules.PSDesiredStateConfigurationMinVersion,
                maxVersion: Modules.PSDesiredStateConfigurationMaxVersion);
        }

        /// <inheritdoc/>
        public ModuleSpecification ModuleSpecification { get; private init; }

        /// <inheritdoc/>
        public string GetDscResourceCmd { get; } = Commands.GetDscResource;

        /// <inheritdoc/>
        public string InvokeDscResourceCmd { get; } = Commands.InvokeDscResource;

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetAllDscResources(PowerShell pwsh)
        {
            return this.GetDscResources(pwsh, null, null);
        }

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetDscResourcesInModule(
            PowerShell pwsh,
            ModuleSpecification moduleSpecification)
        {
            return this.GetDscResources(pwsh, null, moduleSpecification);
        }

        /// <inheritdoc/>
        public DscResourceInfoInternal? GetDscResource(
            PowerShell pwsh,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            var dscResourceInfos = this.GetDscResources(pwsh, name, moduleSpecification);

            if (dscResourceInfos.Count == 0)
            {
                return null;
            }
            else if (dscResourceInfos.Count > 1)
            {
                throw new GetDscResourceMultipleMatches(name, moduleSpecification);
            }

            return dscResourceInfos[0];
        }

        /// <inheritdoc/>
        public ValueSet InvokeGetResource(
            PowerShell pwsh,
            ValueSet settings,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            PSObject? getResult = null;

            try
            {
                getResult = pwsh.AddCommand(this.InvokeDscResourceCmd)
                                .AddParameters(PrepareInvokeParameters(name, settings, moduleSpecification))
                                .AddParameter(Parameters.Method, DscMethods.Get)
                                .InvokeAndStopOnError()
                                .FirstOrDefault();
            }
            catch (Exception ex)
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Get, name, moduleSpecification, ex);
            }

            string? errorMessage = pwsh.GetErrorMessage();
            if (errorMessage is not null)
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Get, name, moduleSpecification, errorMessage);
            }

            if (getResult is null)
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Get, name, moduleSpecification);
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
            PowerShell pwsh,
            ValueSet settings,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            // Returned type is InvokeDscResourceTestResult which is a PowerShell classed defined
            // in PSDesiredStateConfiguration.psm1.
            dynamic? testResult = null;

            try
            {
                testResult = pwsh.AddCommand(this.InvokeDscResourceCmd)
                                 .AddParameters(PrepareInvokeParameters(name, settings, moduleSpecification))
                                 .AddParameter(Parameters.Method, DscMethods.Test)
                                 .InvokeAndStopOnError()
                                 .FirstOrDefault();
            }
            catch (Exception ex)
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Test, name, moduleSpecification, ex);
            }

            string? errorMessage = pwsh.GetErrorMessage();
            if (errorMessage is not null)
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Test, name, moduleSpecification, errorMessage);
            }

            if (testResult is null ||
                !TypeHelpers.PropertyWithTypeExists<bool>(testResult, InDesiredState))
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Test, name, moduleSpecification);
            }

            return testResult?.InDesiredState;
        }

        /// <inheritdoc/>
        public bool InvokeSetResource(
            PowerShell pwsh,
            ValueSet settings,
            string name,
            ModuleSpecification? moduleSpecification)
        {
            // Returned type is InvokeDscResourceSetResult which is a PowerShell classed defined
            // in PSDesiredStateConfiguration.psm1.
            dynamic? setResult = null;

            try
            {
                setResult = pwsh.AddCommand(this.InvokeDscResourceCmd)
                                .AddParameters(PrepareInvokeParameters(name, settings, moduleSpecification))
                                .AddParameter(Parameters.Method, DscMethods.Set)
                                .InvokeAndStopOnError()
                                .FirstOrDefault();
            }
            catch (Exception ex)
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Set, name, moduleSpecification, ex);
            }

            string? errorMessage = pwsh.GetErrorMessage();
            if (errorMessage is not null)
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Set, name, moduleSpecification, errorMessage, pwsh.ContainsPropertyError());
            }

            if (setResult is null ||
                !TypeHelpers.PropertyWithTypeExists<bool>(setResult, RebootRequired))
            {
                throw new InvokeDscResourceException(InvokeDscResourceException.Set, name, moduleSpecification);
            }

            return setResult?.RebootRequired;
        }

        private static Dictionary<string, object> PrepareInvokeParameters(
            string name,
            ValueSet settings,
            ModuleSpecification? moduleSpecification)
        {
            Hashtable properties = settings.ToHashtable();

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

        private IReadOnlyList<DscResourceInfoInternal> GetDscResources(
            PowerShell pwsh,
            string? name,
            ModuleSpecification? moduleSpecification)
        {
            pwsh.AddCommand(this.GetDscResourceCmd);

            if (name is not null)
            {
                pwsh.AddParameter(Parameters.Name, name);
            }

            if (moduleSpecification is not null)
            {
                pwsh.AddParameter(Parameters.Module, moduleSpecification);
            }

            try
            {
                var resources = pwsh.Invoke();
                return this.ConvertToDscResourceInfoInternal(resources);
            }
            catch (RuntimeException e)
            {
                // Detect easily this.
                if (e.ErrorRecord.FullyQualifiedErrorId == "ExceptionWhenSetting,GetResourceFromKeyword")
                {
                    throw new GetDscResourceModuleConflict(name, moduleSpecification, e);
                }

                throw;
            }
        }

        private List<DscResourceInfoInternal> ConvertToDscResourceInfoInternal(Collection<PSObject> psObjects)
        {
            var result = new List<DscResourceInfoInternal>();
            foreach (dynamic psObject in psObjects)
            {
                var dscResourceInfo = new DscResourceInfoInternal(psObject);

                // Explicitly don't support old DSC resources from v1 PSDesiredStateConfiguration.
                // Even if the Windows System32 Windows PowerShell module path is removed they
                // will show up.
                if (ExclusionResourcesParentPath.Any(e => dscResourceInfo.ParentPath!.StartsWith(e)))
                {
                    continue;
                }

                result.Add(dscResourceInfo);
            }

            return result;
        }
    }
}
