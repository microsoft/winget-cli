// -----------------------------------------------------------------------------
// <copyright file="ProcessorEnvironmentFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.ProcessorEnvironments
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.Runspaces;
    using Microsoft.PowerShell;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// Factory class to create a processor environment.
    /// </summary>
    internal class ProcessorEnvironmentFactory
    {
        private readonly ConfigurationProcessorType type;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessorEnvironmentFactory"/> class.
        /// </summary>
        /// <param name="type">Configuration processor type.</param>
        public ProcessorEnvironmentFactory(ConfigurationProcessorType type)
        {
            this.type = type;
        }

        /// <summary>
        /// Create process environment.
        /// </summary>
        /// <returns>IProcessorEnvironment.</returns>
        public IProcessorEnvironment CreateEnvironment()
        {
            return this.CreateProcessorEnvironment();
        }

        private IProcessorEnvironment CreateProcessorEnvironment()
        {
            IDscModule dscModule = new DscModuleV2();

            if (this.type == ConfigurationProcessorType.Default)
            {
                throw new NotImplementedException();
            }
            else if (this.type == ConfigurationProcessorType.Hosted)
            {
                InitialSessionState initialSessionState = InitialSessionState.CreateDefault();

                // If this call fails importing the module, it won't throw but write to the error output. DSCModule is
                // in charge of verifying that it got loaded correctly and if not, to install it. Once logging is implemented
                // we should log the Error PSVariable.
                initialSessionState.ImportPSModule(new List<ModuleSpecification>()
                {
                    dscModule.ModuleSpecification,
                });

                // This is where our policy will get translated to PowerShell's execution policy.
                initialSessionState.ExecutionPolicy = ExecutionPolicy.Unrestricted;

                // The $PSHome\Modules directory is added by default in the modules path. Because this is a hosted PowerShell,
                // we don't have all the nice things that PowerShell installs by default. This includes PowerShellGet.
                var runspace = RunspaceFactory.CreateRunspace(initialSessionState);
                runspace.Open();

                return new HostedEnvironment(runspace, this.type, dscModule);
            }

            throw new ArgumentException(this.type.ToString());
        }
    }
}
