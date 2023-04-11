// -----------------------------------------------------------------------------
// <copyright file="BaseCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;

    /// <summary>
    /// Base class for all Cmdlets.
    /// </summary>
    public abstract class BaseCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="BaseCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal BaseCommand(PSCmdlet psCmdlet)
            : base()
        {
            // The inproc COM API may deadlock on an STA thread.
            if (Utilities.UsesInProcWinget && Utilities.ThreadIsSTA)
            {
                throw new SingleThreadedApartmentException();
            }

            this.PsCmdlet = psCmdlet;
        }

        /// <summary>
        /// Gets the caller PSCmdlet.
        /// </summary>
        protected PSCmdlet PsCmdlet { get; private set; }
    }
}
