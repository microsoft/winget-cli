// -----------------------------------------------------------------------------
// <copyright file="UserSettingsReadException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Exceptions
{
    using System;
    using Microsoft.WinGet.Client.Properties;

    /// <summary>
    /// Settings.json file is invalid.
    /// </summary>
    [Serializable]
    public class UserSettingsReadException : Exception
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
