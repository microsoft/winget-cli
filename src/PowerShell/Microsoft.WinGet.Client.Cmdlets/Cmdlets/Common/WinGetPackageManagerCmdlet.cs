// -----------------------------------------------------------------------------
// <copyright file="WinGetPackageManagerCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// Common parameters for Assert-WinGetPackageManager and Repair-WinGetPackageManager.
    /// </summary>
    public abstract class WinGetPackageManagerCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the optional version.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.IntegrityVersionSet,
            ValueFromPipelineByPropertyName = true)]
        public string Version { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets a value indicating whether to use latest.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.IntegrityLatestSet,
            ValueFromPipelineByPropertyName = true)]
        public SwitchParameter Latest { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to include prerelease winget versions.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.IntegrityLatestSet,
            ValueFromPipelineByPropertyName = true)]
        public SwitchParameter IncludePrerelease { get; set; }
    }
}
