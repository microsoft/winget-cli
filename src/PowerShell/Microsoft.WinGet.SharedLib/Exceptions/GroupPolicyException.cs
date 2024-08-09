// -----------------------------------------------------------------------------
// <copyright file="GroupPolicyException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.SharedLib.Exceptions
{
    using System;
    using Microsoft.WinGet.SharedLib.Extensions;
    using Microsoft.WinGet.SharedLib.PolicySettings;

    /// <summary>
    /// Class that implements GroupPolicyException.
    /// </summary>
    [Serializable]
    public class GroupPolicyException : Exception
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="GroupPolicyException"/> class.
        /// </summary>
        /// <param name="policy">Policy.</param>
        /// <param name="policyFailureType">GroupPolicyFailureType.</param>
        public GroupPolicyException(Policy policy, GroupPolicyFailureType policyFailureType)
            : base(string.Format(policyFailureType.GetFailureString(), policy.GetResourceString()))
        {
            this.HResult = policyFailureType.GetErrorCode();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GroupPolicyException"/> class.
        /// </summary>
        /// <param name="policyFailureType">GroupPolicyFailureType.</param>
        /// <param name="innerException">InnerException.</param>
        public GroupPolicyException(GroupPolicyFailureType policyFailureType, Exception innerException)
            : base(policyFailureType.GetFailureString(), innerException)
        {
            this.HResult = policyFailureType.GetErrorCode();
        }
    }
}
