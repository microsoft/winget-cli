// -----------------------------------------------------------------------------
// <copyright file="IDSCv3.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Model
{
    using Microsoft.Management.Configuration.Processor.DSCv3.Helpers;

    /// <summary>
    /// Interface for interacting with DSC v3.
    /// </summary>
    internal interface IDSCv3
    {
        /// <summary>
        /// Creates the appropriate instance of the DSCv3 interface for the given executable.
        /// </summary>
        /// <param name="processorSettings">The processor settings.</param>
        /// <returns>An object that properly interacts with the specific version of DSC v3.</returns>
        public static IDSCv3 Create(ProcessorSettings processorSettings)
        {
            // Expand as needed to detect the version of dsc.exe and/or its schemas in use.
            return new Schema_2024_04.DSCv3(processorSettings);
        }

        /// <summary>
        /// Gets a single resource by its type name.
        /// </summary>
        /// <param name="resourceType">The type name of the resource.</param>
        /// <returns>A single resource item.</returns>
        public IResourceListItem? GetResourceByType(string resourceType);
    }
}
