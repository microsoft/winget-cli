// -----------------------------------------------------------------------------
// <copyright file="BaseCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.SharedLib.PolicySettings;

    /// <summary>
    /// Base class for all Cmdlets.
    /// </summary>
    public abstract class BaseCommand : PowerShellCmdlet
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BaseCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal BaseCommand(PSCmdlet psCmdlet)
            : base(psCmdlet, new HashSet<Policy> { Policy.WinGet, Policy.WinGetCommandLineInterfaces })
        {
        }
    }
}
