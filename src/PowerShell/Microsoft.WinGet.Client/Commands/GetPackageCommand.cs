// -----------------------------------------------------------------------------
// <copyright file="GetPackageCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Searches configured sources for packages.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.PackageNoun)]
    [OutputType(typeof(CatalogPackage))]
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
                this.WriteObject(results[i].CatalogPackage);
            }
        }
    }
}
