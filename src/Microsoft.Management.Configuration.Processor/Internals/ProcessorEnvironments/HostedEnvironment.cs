// -----------------------------------------------------------------------------
// <copyright file="HostedEnvironment.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Internals.Runspaces
{
    using System;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.Internals.DscModule;
    using Microsoft.PowerShell;
    using static Microsoft.Management.Configuration.Processor.Internals.Constants.PowerShellConstants;

    /// <summary>
    /// Hosted environment.
    /// </summary>
    internal sealed class HostedEnvironment : ProcessorEnvironmentBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="HostedEnvironment"/> class.
        /// </summary>
        /// <param name="dscModule">IDscModule.</param>
        public HostedEnvironment(IDscModule dscModule)
            : base(SetupRunspace(dscModule), dscModule)
        {
            this.Runspace.Open();
            this.ValidateRunspace();
        }

        /// <inheritdoc/>
        public override void ValidateRunspace()
        {
            // Only support PowerShell Core.
            if ((string)this.Runspace.SessionStateProxy.PSVariable.GetValue(Variables.PSEdition) != Core)
            {
                throw new NotSupportedException();
            }

            this.DscModule.ValidateModule(this.Runspace);
        }

        private static Runspace SetupRunspace(IDscModule dscModule)
        {
            // Once we have an stable pre-release, we can look into the scope of if by specifically
            // saying which things should be loaded.
            InitialSessionState initialSessionState = InitialSessionState.CreateDefault();

            // This is where our policy will get translated to PowerShell's execution policy.
            initialSessionState.ExecutionPolicy = ExecutionPolicy.Bypass;

            // Import modules from other paths via initialSessionState.ImportPSModule and ImportPSModulesFromPath
            // Once we have a location for our required Modules use ImportPSModulesFromPath.
            initialSessionState.ImportPSModule(new string[]
            {
                dscModule.ModuleName,
            });

            // The $PSHome\Modules directory is added by default in the modules path. Because this is a hosted PowerShell,
            // we don't have all the nice things that PowerShell installs by default. This includes PowerShellGet.
            // We could look into finding out if PowerShell Core is installed and get their $PSHome\Modules path, but
            // then we will need to remove our own $PSHome\Modules path because there will be some
            // System.Diagnostics.Eventing.Reader.ProviderMetadata errors because member will already be present.
            var runspace = RunspaceFactory.CreateRunspace(initialSessionState);
            return runspace;
        }
    }
}
