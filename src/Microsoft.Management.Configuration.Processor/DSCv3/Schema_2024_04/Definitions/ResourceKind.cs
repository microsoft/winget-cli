// -----------------------------------------------------------------------------
// <copyright file="ResourceKind.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Schema_2024_04.Definitions
{
    /// <summary>
    /// https://learn.microsoft.com/en-us/powershell/dsc/reference/schemas/definitions/resourcekind?view=dsc-3.0
    /// The kind of resource.
    /// </summary>
    internal enum ResourceKind
    {
        /// <summary>
        /// The kind is unknown.
        /// </summary>
        Unknown,

        /// <summary>
        /// A standard resource.
        /// </summary>
        Resource,

        /// <summary>
        /// An adapter resource.
        /// </summary>
        Adapter,

        /// <summary>
        /// A group resource.
        /// </summary>
        Group,

        /// <summary>
        /// An import resource.
        /// </summary>
        Import,
    }
}
