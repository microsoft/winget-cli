// -----------------------------------------------------------------------------
// <copyright file="GetSourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Retrieves the list of configured sources.
    /// </summary>
    [Cmdlet(VerbsCommon.Get, Constants.SourceNoun)]
    [OutputType(typeof(PackageCatalogReference))]
    public sealed class GetSourceCommand : BaseClientCommand
    {
        /// <summary>
        /// Gets or sets the name of the source to retrieve.
        /// </summary>
        [Parameter(
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Returns the list of configured sources.
        /// </summary>
        protected override void ProcessRecord()
        {
            base.ProcessRecord();
            var results = GetPackageCatalogReferences(this.Name);
            for (var i = 0; i < results.Count; i++)
            {
                this.WriteObject(results[i]);
            }
        }
    }
}
