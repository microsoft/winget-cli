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
        private readonly string name;
        private readonly string argument;
        private readonly string type;

        /// <summary>
        /// Initializes a new instance of the <see cref="SourceResult"/> class.
        /// </summary>
        /// <param name="catalogReference">The PackageCatalogReference COM object.</param>
        public SourceResult(Management.Deployment.PackageCatalogReference catalogReference)
        {
            var info = catalogReference.Info;
            this.name = info.Name;
            this.argument = info.Argument;
            this.type = info.Type;
        }

        /// <summary>
        /// Gets the name of the source.
        /// </summary>
        public string Name
        {
            get { return this.name; }
        }

        /// <summary>
        /// Gets the argument of the source.
        /// </summary>
        public string Argument
        {
            get { return this.argument; }
        }

        /// <summary>
        /// Gets the type of the source.
        /// </summary>
        public string Type
        {
            get { return this.type; }
        }
    }
}
