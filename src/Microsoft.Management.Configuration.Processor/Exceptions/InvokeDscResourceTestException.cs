// -----------------------------------------------------------------------------
// <copyright file="InvokeDscResourceTestException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.Exceptions
{
    using System;
    using Microsoft.PowerShell.Commands;

    /// <summary>
    /// A call to Invoke-DscResource Test failed unexpectedly.
    /// </summary>
    internal class InvokeDscResourceTestException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="InvokeDscResourceTestException"/> class.
        /// </summary>
        /// <param name="resourceName">Resource name.</param>
        /// <param name="module">Optional module.</param>
        public InvokeDscResourceTestException(string resourceName, ModuleSpecification? module)
        {
            this.HResult = ErrorCodes.WinGetConfigUnitInvokeTest;
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
