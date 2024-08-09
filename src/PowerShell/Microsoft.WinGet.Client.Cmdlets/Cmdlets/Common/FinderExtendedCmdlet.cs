// -----------------------------------------------------------------------------
// <copyright file="FinderExtendedCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;

    /// <summary>
    /// This is the base class for the commands whose sole purpose is to filter a list of packages i.e.,
    /// the "search" and "list" commands. This class contains an extended set of parameters suited for
    /// that purpose.
    /// </summary>
    public abstract class FinderExtendedCmdlet : FinderCmdlet
    {
        /// <summary>
        /// Gets or sets the filter that is matched against the tags of the package.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Tag { get; set; }

        /// <summary>
        /// Gets or sets the filter that is matched against the commands of the package.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Command { get; set; }

        /// <summary>
        /// Gets or sets the maximum number of results returned.
        /// </summary>
        [ValidateRange(Constants.CountLowerBound, Constants.CountUpperBound)]
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public uint Count { get; set; }
    }
}
