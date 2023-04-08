// -----------------------------------------------------------------------------
// <copyright file="ExecuteAsSystemException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using Microsoft.WinGet.Client.Engine.Properties;

    /// <summary>
    /// Executing as system is disabled.
    /// </summary>
    [Serializable]
    public class ExecuteAsSystemException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ExecuteAsSystemException"/> class.
        /// </summary>
        public ExecuteAsSystemException()
            : base(Resources.ExecuteAsSystemExceptionMessage)
        {
        }
    }
}
