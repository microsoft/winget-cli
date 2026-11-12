// -----------------------------------------------------------------------------
// <copyright file="ExcludedSources.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    /// <summary>
    /// The sources that were not used for a request.
    /// </summary>
    internal sealed class ExcludedSources
    {
        /// <summary>
        /// Gets the names of the sources whose agreements the user has not accepted.
        /// </summary>
        public List<string> NotAccepted { get; } = new List<string>();

        /// <summary>
        /// Gets the names of the sources whose agreement state could not be determined. These are not used,
        /// as doing so could bypass the user's consent once the source becomes reachable.
        /// </summary>
        public List<string> Unavailable { get; } = new List<string>();

        /// <summary>
        /// Gets a value indicating whether no sources were excluded.
        /// </summary>
        public bool IsEmpty => NotAccepted.Count == 0 && Unavailable.Count == 0;

        /// <summary>
        /// Adds the sources excluded by another request.
        /// </summary>
        /// <param name="other">The other excluded sources.</param>
        public void AddRange(ExcludedSources other)
        {
            NotAccepted.AddRange(other.NotAccepted);
            Unavailable.AddRange(other.Unavailable);
        }
    }
}
