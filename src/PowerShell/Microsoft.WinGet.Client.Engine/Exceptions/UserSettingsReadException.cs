// -----------------------------------------------------------------------------
// <copyright file="UserSettingsReadException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Settings.json file is invalid.
    /// </summary>
    [Serializable]
    public class UserSettingsReadException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UserSettingsReadException"/> class.
        /// </summary>
        public UserSettingsReadException()
            : base(Resources.UserSettingsReadException)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="UserSettingsReadException"/> class.
        /// </summary>
        /// <param name="inner">Inner exception.</param>
        public UserSettingsReadException(Exception inner)
            : base(Resources.UserSettingsReadException, inner)
        {
        }
    }
}
