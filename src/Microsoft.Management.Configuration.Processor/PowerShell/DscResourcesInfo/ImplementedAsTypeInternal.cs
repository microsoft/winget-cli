// -----------------------------------------------------------------------------
// <copyright file="ImplementedAsTypeInternal.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo
{
    /// <summary>
    /// Enumerated values for DSC resource implementation type.
    /// </summary>
    internal enum ImplementedAsTypeInternal
    {
        /// <summary>
        /// DSC resource implementation type not known.
        /// </summary>
        None = 0,

        /// <summary>
        /// DSC resource is implemented using PowerShell module.
        /// </summary>
        PowerShell = 1,

        /// <summary>
        /// DSC resource is implemented using a CIM provider.
        /// </summary>
        Binary = 2,

        /// <summary>
        /// DSC resource is a composite and implemented using configuration keyword.
        /// </summary>
        Composite = 3,
    }
}
