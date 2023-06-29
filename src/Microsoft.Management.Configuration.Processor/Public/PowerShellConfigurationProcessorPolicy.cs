// -----------------------------------------------------------------------------
// <copyright file="PowerShellConfigurationProcessorPolicy.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    /// <summary>
    /// Processor policy.
    /// For Processor type Default and Hosted they mean the same as PowerShell ExecutionPolicy.
    /// https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_execution_policies.
    /// </summary>
    public enum PowerShellConfigurationProcessorPolicy
    {
        /// <summary>
        /// Unrestricted.
        /// </summary>
        Unrestricted = 0,

        /// <summary>
        /// RemoteSigned.
        /// </summary>
        RemoteSigned = 1,

        /// <summary>
        /// AllSigned.
        /// </summary>
        AllSigned = 2,

        /// <summary>
        /// Restricted.
        /// </summary>
        Restricted = 3,

        /// <summary>
        /// Bypass.
        /// </summary>
        Bypass = 4,

        /// <summary>
        /// Undefined.
        /// </summary>
        Undefined = 5,

        /// <summary>
        /// Default.
        /// </summary>
        Default = RemoteSigned,
    }
}
