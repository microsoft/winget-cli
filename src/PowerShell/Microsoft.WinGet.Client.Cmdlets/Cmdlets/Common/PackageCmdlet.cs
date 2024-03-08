// -----------------------------------------------------------------------------
// <copyright file="PackageCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.Engine.PSObjects;

    /// <summary>
    /// This is the base class for commands which operate on a specific package and version i.e.,
    /// the "install", "uninstall", and "upgrade" commands.
    /// </summary>
    public abstract class PackageCmdlet : FinderCmdlet
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PackageCmdlet"/> class.
        /// </summary>
        public PackageCmdlet()
        {
            // The default match option for single package operations.
            this.MatchOption = PSObjects.PSPackageFieldMatchOption.EqualsCaseInsensitive;
        }

        /// <summary>
        /// Gets or sets the package to directly install.
        /// </summary>
        [Alias("InputObject")]
        [ValidateNotNull]
        [Parameter(
            ParameterSetName = Constants.GivenSet,
            Position = 0,
            ValueFromPipeline = true,
            ValueFromPipelineByPropertyName = true)]
        public PSCatalogPackage PSCatalogPackage { get; set; } = null;

        /// <summary>
        /// Gets or sets the version to install.
        /// </summary>
        [Parameter(ValueFromPipelineByPropertyName = true)]
        public string Version { get; set; }
    }
}
