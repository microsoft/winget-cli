// -----------------------------------------------------------------------------
// <copyright file="FinderCmdlet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Commands.Common
{
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Common;
    using Microsoft.WinGet.Client.PSObjects;

    /// <summary>
    /// This is the base class for all commands that might need to search for a package. It contains an initial
    /// set of parameters that corresponds to the intersection of i.e., the "install" and "search" commands.
    /// </summary>
    public abstract class FinderCmdlet : PSCmdlet
    {
        /// <summary>
        /// Gets or sets the field that is matched against the identifier of a package.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Id { get; set; }

        /// <summary>
        /// Gets or sets the field that is matched against the name of a package.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Name { get; set; }

        /// <summary>
        /// Gets or sets the field that is matched against the moniker of a package.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Moniker { get; set; }

        /// <summary>
        /// Gets or sets the name of the source to search for packages. If null, then all sources are searched.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public string Source { get; set; }

        /// <summary>
        /// Gets or sets the strings that match against every field of a package.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            Position = 0,
            ValueFromPipelineByPropertyName = true,
            ValueFromRemainingArguments = true)]
        public string[] Query { get; set; }

        /// <summary>
        /// Gets or sets how to match against package fields. Default ContainsCaseInsensitive.
        /// </summary>
        [Parameter(
            ParameterSetName = Constants.FoundSet,
            ValueFromPipelineByPropertyName = true)]
        public PSPackageFieldMatchOption MatchOption { get; set; } = PSPackageFieldMatchOption.ContainsCaseInsensitive;
    }
}
