// -----------------------------------------------------------------------------
// <copyright file="WinGetRepairException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Exceptions
{
    using System;
    using System.Management.Automation;
    using Microsoft.WinGet.Client.Engine.Common;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// WinGet repair exception.
    /// </summary>
    [Serializable]
    public class WinGetRepairException : RuntimeException
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairException"/> class.
        /// </summary>
        /// <param name="ie">Integrity exception.</param>
        public WinGetRepairException(WinGetIntegrityException ie)
            : base(GetMessage(ie), ie)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairException"/> class.
        /// </summary>
        /// <param name="e">Inner exception.</param>
        public WinGetRepairException(Exception e)
            : base(Resources.RepairFailureMessage, e)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairException"/> class.
        /// </summary>
        public WinGetRepairException()
            : base(Resources.RepairFailureMessage)
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="WinGetRepairException"/> class.
        /// </summary>
        /// <param name="message">Message.</param>.
        public WinGetRepairException(string message)
            : base(message)
        {
        }

        private static string GetMessage(WinGetIntegrityException ie)
        {
            string message = Resources.RepairFailureMessage;
            if (ie.Category == IntegrityCategory.AppInstallerNoLicense)
            {
                message += $" {Resources.RepairAllUsersHelpMessage}";
            }
            else if (ie.Category == IntegrityCategory.AppExecutionAliasDisabled)
            {
                message += $" {Resources.RepairAppExecutionAliasMessage}";
            }

            return message;
        }
    }
}
