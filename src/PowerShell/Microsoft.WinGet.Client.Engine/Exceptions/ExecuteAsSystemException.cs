﻿// -----------------------------------------------------------------------------
// <copyright file="SingleThreadedApartmentException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
<<<<<<< HEAD:src/PowerShell/Microsoft.WinGet.Client.Engine/Exceptions/ExecuteAsSystemException.cs
    using Microsoft.WinGet.Client.Engine.Properties;
=======
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Properties;
>>>>>>> upstream/master:src/PowerShell/Microsoft.WinGet.Client/Exceptions/SingleThreadedApartmentException.cs

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
