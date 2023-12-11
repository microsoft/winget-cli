// -----------------------------------------------------------------------------
// <copyright file="ReleaseExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Extensions
{
    using System.Linq;
    using Microsoft.WinGet.Client.Engine.Exceptions;
    using Microsoft.WinGet.Resources;
    using Octokit;

    /// <summary>
    /// Extension methods for Octokit.Release.
    /// </summary>
    internal static class ReleaseExtensions
    {
        /// <summary>
        /// Gets the Asset.
        /// </summary>
        /// <param name="release">GitHub release.</param>
        /// <param name="name">Name of asset.</param>
        /// <returns>The asset.</returns>
        public static ReleaseAsset GetAsset(this Release release, string name)
        {
            var assets = release.Assets.Where(a => a.Name == name);

            if (assets.Any())
            {
                return assets.First();
            }

            throw new WinGetRepairException(string.Format(Resources.ReleaseAssetNotFound, name));
        }

        /// <summary>
        /// Gets the asset that ends with the string.
        /// </summary>
        /// <param name="release">GitHub release.</param>
        /// <param name="name">Asset last part name.</param>
        /// <returns>The asset.</returns>
        public static ReleaseAsset GetAssetEndsWith(this Release release, string name)
        {
            var assets = release.Assets.Where(a => a.Name.EndsWith(name));

            if (assets.Any())
            {
                return assets.First();
            }

            throw new WinGetRepairException(string.Format(Resources.ReleaseAssetNotFound, name));
        }
    }
}
