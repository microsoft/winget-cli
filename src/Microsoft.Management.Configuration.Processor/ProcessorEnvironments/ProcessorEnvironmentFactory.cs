// -----------------------------------------------------------------------------
// <copyright file="ProcessorEnvironmentFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.ProcessorEnvironments
{
    using System;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.Runspaces;
    using Microsoft.PowerShell;

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
            var runspace = this.GetRunspace();

            var processEnvironment = new ProcessorEnvironment(
                runspace,
                this.type,
                new DscModuleV2());

            processEnvironment.ValidateRunspace();

            return processEnvironment;
        }

        private Runspace GetRunspace()
        {
            if (this.type == ConfigurationProcessorType.Default)
            {
                return Runspace.DefaultRunspace;
            }
            else if (this.type == ConfigurationProcessorType.Hosted)
            {
                // Once we have an stable pre-release, we can look into the scope of if by specifically
                // saying which things should be loaded.
                InitialSessionState initialSessionState = InitialSessionState.CreateDefault();

                // This is where our policy will get translated to PowerShell's execution policy.
                initialSessionState.ExecutionPolicy = ExecutionPolicy.Bypass;

                // The $PSHome\Modules directory is added by default in the modules path. Because this is a hosted PowerShell,
                // we don't have all the nice things that PowerShell installs by default. This includes PowerShellGet.
                // We could look into finding out if PowerShell Core is installed and get their $PSHome\Modules path, but
                // then we will need to remove our own $PSHome\Modules path because there will be some
                // System.Diagnostics.Eventing.Reader.ProviderMetadata errors because member will already be present.
                var runspace = RunspaceFactory.CreateRunspace(initialSessionState);
                runspace.Open();
                return runspace;
            }

            throw new ArgumentException(this.type.ToString());
        }
    }
}
