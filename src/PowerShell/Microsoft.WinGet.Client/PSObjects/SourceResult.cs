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
        /// The name of the source.
        /// </summary>
        public readonly string Name;

        /// <summary>
        /// The argument of the source.
        /// </summary>
        public readonly string Argument;

        /// <summary>
        /// The type of the source.
        /// </summary>
        public readonly string Type;

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
    }
}
