// -----------------------------------------------------------------------------
// <copyright file="CatalogConnectException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Failed connecting to catalog.
    /// </summary>
    [Serializable]
    public class CatalogConnectException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="CatalogConnectException"/> class.
        /// </summary>
        /// <param name="inner">The exception that lead to this one.</param>
        public CatalogConnectException(Exception inner)
            : base(Resources.CatalogConnectExceptionMessage, inner)
        {
        }
    }
}
