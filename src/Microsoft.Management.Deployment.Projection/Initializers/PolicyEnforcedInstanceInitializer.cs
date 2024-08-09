// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace Microsoft.Management.Deployment.Projection.Initializers
{
    using Microsoft.WinGet.SharedLib.Exceptions;
    using Microsoft.WinGet.SharedLib.PolicySettings;

    /// <summary>
    /// An abstract base that enforces group policy before creating derived class instance.
    /// </summary>
    public abstract class PolicyEnforcedInstanceInitializer : IInstanceInitializer
    {
        /// <summary>
        /// CLSID context.
        /// </summary>
        public abstract ClsidContext Context { get; }

        /// <summary>
        /// Create instance of the provided type.
        /// </summary>
        /// <typeparam name="T">Projected class type.</typeparam>
        /// <returns>Instance of the provided type.</returns>
        public T CreateInstance<T>() where T : new()
        {
            GroupPolicy groupPolicy = GroupPolicy.GetInstance();

            if (!groupPolicy.IsEnabled(Policy.WinGet))
            {
                throw new GroupPolicyException(Policy.WinGet, GroupPolicyFailureType.BlockedByPolicy);
            }

            return this.CreateInstanceInternal<T>();
        }

        protected abstract T CreateInstanceInternal<T>() where T : new();
    }
}
