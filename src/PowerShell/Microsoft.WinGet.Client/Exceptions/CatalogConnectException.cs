// -----------------------------------------------------------------------------
// <copyright file="CatalogConnectException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Failed connecting to catalog.
    /// </summary>
    [Serializable]
    public class CatalogConnectException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CatalogConnectException"/> class.
        /// </summary>
        public CatalogConnectException()
            : base(Resources.CatalogConnectExceptionMessage)
        {
        }
    }
}
