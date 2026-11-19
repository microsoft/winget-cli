// -----------------------------------------------------------------------------
// <copyright file="NoSourcesAvailableException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// No source could be used because none of their agreements were accepted or readable.
    /// </summary>
    [Serializable]
    public class NoSourcesAvailableException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="NoSourcesAvailableException"/> class.
        /// </summary>
        public NoSourcesAvailableException()
            : base(Resources.NoSourcesAvailableExceptionMessage)
        {
        }
    }
}
