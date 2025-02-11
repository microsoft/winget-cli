// -----------------------------------------------------------------------------
// <copyright file="ProcessorEnvironmentFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscModules;
    using Microsoft.Management.Configuration.Processor.PowerShell.Runspaces;
    using Microsoft.PowerShell;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// Factory class to create a processor environment.
    /// </summary>
    internal class ProcessorEnvironmentFactory
    {
        private readonly PowerShellConfigurationProcessorType type;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessorEnvironmentFactory"/> class.
        /// </summary>
        /// <param name="type">Configuration processor type.</param>
        public ProcessorEnvironmentFactory(PowerShellConfigurationProcessorType type)
        {
            this.type = type;
        }

        /// <summary>
        /// Create process environment.
        /// </summary>
        /// <param name="setProcessorFactory">Optional processor factory.</param>
        /// <param name="policy">Configuration processor policy.</param>
        /// <returns>IProcessorEnvironment.</returns>
        public IProcessorEnvironment CreateEnvironment(
            PowerShellConfigurationSetProcessorFactory? setProcessorFactory,
            PowerShellConfigurationProcessorPolicy policy)
        {
            IDscModule dscModule = new DscModuleV2();
            ExecutionPolicy executionPolicy = this.GetExecutionPolicy(policy);

            // The for ConfigurationProcessorType.Default the idea was that since is already running in PowerShell we will
            // have access to the variables in the current runspace, but we can't use that runspace and AFAIK
            // there's not a simple way to simply clone a runspace. If we want to do it, we will need to get the
            // variables from the current runspace and add them here, but maybe some of them are objects that can't
            // handle being used in different runspace. It will also be time consuming and we can't block for creating
            // the create set processor. Even if we could clone it, at this point we are running in a different thread,
            // so there's no default runspace to clone here (aka. PowerShell.Create(RunspaceMode.CurrentRunspace) throws)
            //
            // If we want to somehow support, it might be easier to explicitly ask for the variables that need to be
            // ported. We can add a new property to IConfigurationProcessorFactoryProperties with the variable names
            // and set them here, but if they change they won't get reflected in our runspace (which might be a good thing).
            // The problem with that is that they will need to be defined when the configuration set is opened and it really
            // just makes sense before the ConfigurationSetProcessor gets created. We could add a new IConfigurationSetProcessorProperties
            // Then in PowerShell it can be something like
            // Get-WinGetConfiguration | Add-WinGetConfigurationVariable -Name foo | Start-WinGetConfiguration
            if (this.type == PowerShellConfigurationProcessorType.Hosted ||
                this.type == PowerShellConfigurationProcessorType.Default)
            {
                var initialSessionState = this.CreateInitialSessionState(
                    executionPolicy,
                    new List<ModuleSpecification>
                    {
                        dscModule.ModuleSpecification,
                    });

                var runspace = RunspaceFactory.CreateRunspace(initialSessionState);
                runspace.Open();

                return new HostedEnvironment(runspace, this.type, dscModule)
                {
                    SetProcessorFactory = setProcessorFactory,
                };
            }

            throw new ArgumentException(this.type.ToString());
        }

        private InitialSessionState CreateInitialSessionState(ExecutionPolicy policy, IReadOnlyList<ModuleSpecification> modules)
        {
            InitialSessionState initialSessionState = InitialSessionState.CreateDefault();

            // If this call fails importing the module, it won't throw but write to the error output. DSCModule is
            // in charge of verifying that it got loaded correctly and if not, to install it.
            initialSessionState.ImportPSModule(modules);

            initialSessionState.ExecutionPolicy = policy;

            return initialSessionState;
        }

        private ExecutionPolicy GetExecutionPolicy(PowerShellConfigurationProcessorPolicy policy)
        {
            return policy switch
            {
                PowerShellConfigurationProcessorPolicy.Unrestricted => ExecutionPolicy.Unrestricted,
                PowerShellConfigurationProcessorPolicy.RemoteSigned => ExecutionPolicy.RemoteSigned,
                PowerShellConfigurationProcessorPolicy.AllSigned => ExecutionPolicy.AllSigned,
                PowerShellConfigurationProcessorPolicy.Restricted => ExecutionPolicy.Restricted,
                PowerShellConfigurationProcessorPolicy.Bypass => ExecutionPolicy.Bypass,
                _ => throw new InvalidOperationException(),
            };
        }
    }
}
