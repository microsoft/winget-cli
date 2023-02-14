// -----------------------------------------------------------------------------
// <copyright file="DefaultEnvironment.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Internals.Runspaces
{
    using System;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.Internals.DscModule;
    using static Microsoft.Management.Configuration.Processor.Internals.Constants.PowerShellConstants;

    /// <summary>
    /// Default processor environment. This should be already running in PowerShell.
    /// </summary>
    internal sealed class DefaultEnvironment : ProcessorEnvironmentBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="DefaultEnvironment"/> class.
        /// </summary>
        /// <param name="dscModule">IDscModule.</param>
        public DefaultEnvironment(IDscModule dscModule)
            : base(Runspace.DefaultRunspace, dscModule)
        {
            // Once we get the variables, we should validate that they are set in the default
            // runspace. Also import any modules not in module paths if needed.
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
    }
}
