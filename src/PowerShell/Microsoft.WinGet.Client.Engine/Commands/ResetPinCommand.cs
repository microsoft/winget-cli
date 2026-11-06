// -----------------------------------------------------------------------------
// <copyright file="ResetPinCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Commands.Common;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Microsoft.WinGet.Client.Engine.PSObjects;
    using Microsoft.WinGet.Common.Command;

    /// <summary>
    /// Engine command for resetting all pins.
    /// </summary>
    public sealed class ResetPinCommand : ManagementDeploymentCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ResetPinCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">Caller cmdlet.</param>
        public ResetPinCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Resets all pins, optionally scoped to a source.
        /// </summary>
        /// <param name="sourceName">The source name to scope the reset. Pass null or empty to reset all sources.</param>
        public void Reset(string sourceName)
        {
            PackageCatalogReference? catalogReference = null;
            if (!string.IsNullOrEmpty(sourceName))
            {
                catalogReference = PackageManagerWrapper.Instance.GetPackageCatalogByName(sourceName)
                    ?? throw new InvalidSourceException(sourceName);
            }

            var result = this.Execute(
                () => PackageManagerWrapper.Instance.ResetAllPins(catalogReference));

            this.Write(StreamType.Object, new PSPinResult(result));
        }
    }
}
