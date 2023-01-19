// -----------------------------------------------------------------------------
// <copyright file="SourceResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// SourceResult wrapper object for displaying to PowerShell.
    /// </summary>
    public class SourceResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SourceResult"/> class.
        /// </summary>
        /// <param name="catalogReference">The PackageCatalogReference COM object.</param>
        public SourceResult(Management.Deployment.PackageCatalogReference catalogReference)
        {
            var info = catalogReference.Info;
            this.Name = info.Name;
            this.Argument = info.Argument;
            this.Type = info.Type;
        }

        /// <summary>
        /// Gets the name of the source.
        /// </summary>
        public string Name { get; private set; }

        /// <summary>
        /// Gets the argument of the source.
        /// </summary>
        public string Argument { get; private set; }

        /// <summary>
        /// Gets the type of the source.
        /// </summary>
        public string Type { get; private set; }
    }
}
