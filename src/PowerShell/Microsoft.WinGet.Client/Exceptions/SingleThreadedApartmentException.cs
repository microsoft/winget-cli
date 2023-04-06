// -----------------------------------------------------------------------------
// <copyright file="SingleThreadedApartmentException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// No package found.
    /// </summary>
    [Serializable]
    public class SingleThreadedApartmentException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="SingleThreadedApartmentException"/> class.
        /// </summary>
        public SingleThreadedApartmentException()
            : base(Resources.SingleThreadedApartmentNotSupportedMessage)
        {
        }
    }
}
