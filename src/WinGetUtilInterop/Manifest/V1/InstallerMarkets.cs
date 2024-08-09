// -----------------------------------------------------------------------------
// <copyright file="InstallerMarkets.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System.Collections.Generic;

    /// <summary>
    /// Class that contains installer markets.
    /// </summary>
    public class InstallerMarkets
    {
        /// <summary>
        /// Gets or sets the list of allowed markets.
        /// </summary>
        public List<string> AllowedMarkets { get; set; }

        /// <summary>
        /// Gets or sets the list of excluded markets.
        /// </summary>
        public List<string> ExcludedMarkets { get; set; }
    }
}