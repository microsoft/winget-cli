// -----------------------------------------------------------------------------
// <copyright file="PSPackageFieldMatchOption.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.PSObjects
{
    /// <summary>
    /// This should mimic Microsoft.Management.Deployment.PackageFileMatchOption.
    /// </summary>
    public enum PSPackageFieldMatchOption
    {
        /// <summary>
        /// Equals.
        /// </summary>
        Equals,

        /// <summary>
        /// EqualsCaseInsensitive.
        /// </summary>
        EqualsCaseInsensitive,

        /// <summary>
        /// StartsWithCaseInsensitive.
        /// </summary>
        StartsWithCaseInsensitive,

        /// <summary>
        /// ContainsCaseInsensitive
        /// </summary>
        ContainsCaseInsensitive,
    }
}
