// -----------------------------------------------------------------------------
// <copyright file="PSCompareResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    /// <summary>
    /// Must match Microsoft.Management.Deployment.CompareResult.
    /// </summary>
    public enum PSCompareResult
    {
        /// <summary>
        /// Unknown,
        /// </summary>
        Unknown,

        /// <summary>
        /// Lesser.
        /// </summary>
        Lesser,

        /// <summary>
        /// Equal,
        /// </summary>
        Equal,

        /// <summary>
        /// Greater,
        /// </summary>
        Greater,
    }
}
