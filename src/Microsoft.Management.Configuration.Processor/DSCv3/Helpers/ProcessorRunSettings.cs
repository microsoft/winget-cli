// -----------------------------------------------------------------------------
// <copyright file="ProcessorRunSettings.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.Helpers;

    /// <summary>
    /// Contains settings for the DSC v3 processor components to share.
    /// </summary>
    internal class ProcessorRunSettings
    {
        /// <summary>
        /// Gets the paths for finding DSC resources and executables.
        /// </summary>
        public string ResourceSearchPaths { get; private set; } = string.Empty;

        /// <summary>
        /// Gets a value indicating whether the resource search paths are exclusive.
        /// </summary>
        public bool ResourceSearchPathsExclusive { get; private set; } = false;

        /// <summary>
        /// Creates ProcessorRunSettings from FindUnitProcessorsOptions.
        /// </summary>
        /// <param name="findOptions">The find unit processors options.</param>
        /// <returns>A ProcessorRunSettings.</returns>
        public static ProcessorRunSettings CreateFromFindUnitProcessorsOptions(FindUnitProcessorsOptions findOptions)
        {
            return new ProcessorRunSettings
            {
                ResourceSearchPaths = findOptions.SearchPaths,
                ResourceSearchPathsExclusive = findOptions.SearchPathsExclusive,
            };
        }

        /// <summary>
        /// Creates ProcessorRunSettings from a ResourceDetails.
        /// </summary>
        /// <param name="resourceDetails">The resource details to be used.</param>
        /// <returns>A ProcessorRunSettings.</returns>
        public static ProcessorRunSettings CreateFromResourceDetails(ResourceDetails? resourceDetails)
        {
            return new ProcessorRunSettings
            {
                ResourceSearchPaths = Path.GetDirectoryName(resourceDetails?.Path) ?? string.Empty,
                ResourceSearchPathsExclusive = false,
            };
        }
    }
}
