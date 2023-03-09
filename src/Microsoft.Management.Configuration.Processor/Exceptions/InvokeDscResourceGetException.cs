// -----------------------------------------------------------------------------
// <copyright file="InvokeDscResourceGetException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// A call to Invoke-DscResource Get failed unexpectedly.
    /// </summary>
    internal class InvokeDscResourceGetException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceGetException"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        public InvokeDscResourceGetException(string resourceName, ModuleSpecification? module)
        {
            this.HResult = ErrorCodes.WinGetConfigUnitInvokeGet;
            this.ResourceName = resourceName;
            this.Module = module;
        }

        /// <summary>
        /// Gets the resource name.
        /// </summary>
        public string ResourceName { get; }

        /// <summary>
        /// Gets the module, if any.
        /// </summary>
        public ModuleSpecification? Module { get; }
    }
}
