﻿// -----------------------------------------------------------------------------
// <copyright file="GetPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// Searches configured sources for packages.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.Package)]
    [OutputType(typeof(PSInstalledCatalogPackage))]
    public sealed class GetPackageCommand : FinderExtendedCommand
    {
        /// <summary>
        /// Searches for configured sources for packages.
        /// </summary>
        protected override void ProcessRecord()
        {
            // TODO: create finderpacakge, call get
        }
    }
}
