// -----------------------------------------------------------------------------
// <copyright file="BaseCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Base
{
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
        public BaseCommand()
            : base()
        {
            // The inproc COM API may deadlock on an STA thread.
            if (Utilities.UsesInProcWinget && Utilities.ThreadIsSTA)
            {
                throw new SingleThreadedApartmentException();
            }
        }
    }
}
