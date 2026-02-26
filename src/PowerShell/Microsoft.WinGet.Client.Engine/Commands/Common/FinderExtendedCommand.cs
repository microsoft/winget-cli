// -----------------------------------------------------------------------------
// <copyright file="FinderExtendedCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Commands.Common
{
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Client.Engine.Attributes;

    /// <summary>
    /// This is the base class for the commands whose sole purpose is to filter a list of packages i.e.,
    /// the "search" and "list" commands. This class contains an extended set of parameters suited for
    /// that purpose.
    /// </summary>
    public abstract class FinderExtendedCommand : FinderCommand
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="FinderExtendedCommand"/> class.
        /// </summary>
        /// <param name="psCmdlet">PSCmdlet.</param>
        internal FinderExtendedCommand(PSCmdlet psCmdlet)
            : base(psCmdlet)
        {
        }

        /// <summary>
        /// Gets or sets the filter that is matched against the tags of the package.
        /// </summary>
        [Filter(Field = PackageMatchField.Tag)]
        protected string? Tag { get; set; }

        /// <summary>
        /// Gets or sets the filter that is matched against the commands of the package.
        /// </summary>
        [Filter(Field = PackageMatchField.Command)]
        protected string? Command { get; set; }

        /// <summary>
        /// Gets or sets the maximum number of results returned.
        /// </summary>
        protected uint Count { get; set; }

        /// <summary>
        /// Searches for packages from configured sources.
        /// </summary>
        /// <param name="behavior">A <see cref="CompositeSearchBehavior" /> value.</param>
        /// <param name="match">The match option.</param>
        /// <returns>A list of <see cref="MatchResult" /> objects.</returns>
        internal IReadOnlyList<MatchResult> FindPackages(CompositeSearchBehavior behavior, PackageFieldMatchOption match)
        {
            return this.FindPackages(behavior, this.Count, match);
        }
    }
}
