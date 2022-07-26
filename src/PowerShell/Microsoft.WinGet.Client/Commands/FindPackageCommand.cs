// -----------------------------------------------------------------------------
// <copyright file="FindPackageCommand.cs" company="Microsoft Corporation">
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
    [Cmdlet(VerbsCommon.Find, Constants.PackageNoun)]
    [OutputType(typeof(MatchResult))]
    public sealed class FindPackageCommand : BaseFinderExtendedCommand
    {
        /// <summary>
        /// Searches for configured sources for packages.
        /// </summary>
        protected override void ProcessRecord()
        {
            base.ProcessRecord();
            var results = this.FindPackages(CompositeSearchBehavior.RemotePackagesFromRemoteCatalogs);
            for (var i = 0; i < results.Count; i++)
            {
                this.WriteObject(results[i]);
            }
        }
    }
}
