// -----------------------------------------------------------------------------
// <copyright file="DscModuleV2.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DscModule
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
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
    /// PSDesiredStateConfiguration v2.0.5.
    /// </summary>
    internal class DscModuleV2 : IDscModule
    {
        private const string ExperimentalFeatureName = "PSDesiredStateConfiguration.InvokeDscResource";
        private const string InDesiredState = "InDesiredState";
        private const string RebootRequired = "RebootRequired";

        private readonly bool customModule = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="DscModuleV2"/> class.
        /// </summary>
        /// <param name="customModule">Is custom module.</param>
        public DscModuleV2(bool customModule)
        {
            this.customModule = customModule;
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
            if (this.customModule)
            {
                return;
            }

            // Make sure we have PSDesiredStateConfiguration installed and that
            // PSDesiredStateConfiguration.InvokeDscResource is enabled.
            if (!ExperimentalFeature.IsEnabled(ExperimentalFeatureName))
            {
                // I want to use Get-InstalledModule, but couldn't make it work. It would just return no result
                // and throw an exception reading the error stream.
                using var pwsh = PowerShell.Create(runspace);

                var getDscResourceCommand = pwsh.AddCommand(Commands.GetCommand)
                                                .AddParameter(Parameters.Name, this.GetDscResourceCmd)
                                                .InvokeAndStopOnError<FunctionInfo>()
                                                .FirstOrDefault();

                if (getDscResourceCommand is null)
                {
                    throw new NotSupportedException($"Module {Modules.PSDesiredStateConfiguration} not found.");
                }

                // Get-DscResource would be found in the PSDesiredStateConfiguration module
                // in the Windows PowerShell module path.
                var supportedVersion = Version.Parse(Modules.PSDesiredStateConfigurationVersion);
                if (getDscResourceCommand.Version.CompareTo(supportedVersion) != 0)
                {
                    throw new NotSupportedException($"Module {Modules.PSDesiredStateConfiguration} version" +
                        $" {getDscResourceCommand.Version} not supported.");
                }
                else
                {
                    // Supported version is installed, but the feature is not enabled.
                    throw new NotSupportedException($"Experimental feature {ExperimentalFeatureName} not enabled.");
                }
            }
        }

        /// <inheritdoc/>
        public IReadOnlyList<DscResourceInfoInternal> GetAllDscResources(Runspace runspace)
        {
            var result = new List<DscResourceInfoInternal>();

            using PowerShell pwsh = PowerShell.Create(runspace);
            var resources = pwsh.AddCommand(this.GetDscResourceCmd)
                                .InvokeAndStopOnError();

            foreach (dynamic resource in resources)
            {
                result.Add(new DscResourceInfoInternal(resource));
            }

            return result;
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

            foreach (dynamic resource in resources)
            {
                result.Add(new DscResourceInfoInternal(resource));
            }

            return result;
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

            if (resources.Count == 0)
            {
                return null;
            }
            else if (resources.Count > 1)
            {
                // Get-DscResource fails before we get here.
                throw new ArgumentException(name);
            }

            return new DscResourceInfoInternal(resources[0]);
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
                                .AddParameter(Parameters.Method, DscMethod.Get)
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
                                      .AddParameter(Parameters.Method, DscMethod.Test)
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
                                     .AddParameter(Parameters.Method, DscMethod.Set)
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
    }
}
