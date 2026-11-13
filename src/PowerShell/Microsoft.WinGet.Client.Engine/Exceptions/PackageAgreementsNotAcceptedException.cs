// -----------------------------------------------------------------------------
// <copyright file="PackageAgreementsNotAcceptedException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// The agreements of a package were not accepted.
    /// </summary>
    [Serializable]
    public class PackageAgreementsNotAcceptedException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PackageAgreementsNotAcceptedException"/> class.
        /// </summary>
        /// <param name="package">The package description.</param>
        public PackageAgreementsNotAcceptedException(string package)
            : base(string.Format(Resources.PackageAgreementsNotAcceptedExceptionMessage, package))
        {
        }
    }
}
