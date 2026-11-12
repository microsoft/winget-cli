// -----------------------------------------------------------------------------
// <copyright file="PackageAgreementsUnavailableException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// The agreements of a package could not be read.
    /// </summary>
    [Serializable]
    public class PackageAgreementsUnavailableException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="PackageAgreementsUnavailableException"/> class.
        /// </summary>
        /// <param name="package">The package description.</param>
        /// <param name="inner">The exception that lead to this one.</param>
        public PackageAgreementsUnavailableException(string package, Exception inner)
            : base(string.Format(Resources.PackageAgreementsUnavailableExceptionMessage, package), inner)
        {
        }
    }
}
