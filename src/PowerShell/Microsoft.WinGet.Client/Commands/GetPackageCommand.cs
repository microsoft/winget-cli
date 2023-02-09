// -----------------------------------------------------------------------------
// <copyright file="GetPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Commands.Common;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Searches configured sources for packages.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.WinGetNouns.Package)]
    [OutputType(typeof(PSObjects.InstalledCatalogPackage))]
    public sealed class GetPackageCommand : BaseFinderExtendedCommand
    {
        /// <summary>
        /// Searches for configured sources for packages.
        /// </summary>
        protected override void ProcessRecord()
        {
            base.ProcessRecord();
            var results = this.FindPackages(CompositeSearchBehavior.LocalCatalogs);
            for (var i = 0; i < results.Count; i++)
            {
                this.WriteObject(new PSObjects.InstalledCatalogPackage(results[i].CatalogPackage));
            }
        }
    }
}
