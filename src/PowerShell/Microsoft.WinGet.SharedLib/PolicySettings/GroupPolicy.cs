// -----------------------------------------------------------------------------
// <copyright file="GroupPolicy.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.SharedLib.PolicySettings
{
    using System;
    using System.Collections.Generic;

    using Microsoft.Win32;
    using Microsoft.WinGet.SharedLib.Exceptions;

    /// <summary>
    /// Helper class to read Group Policies backed by registry store.
    /// </summary>
    public class GroupPolicy
    {
        private const string AppInstallerPolicyRegistryPath = "SOFTWARE\\Policies\\Microsoft\\Windows\\AppInstaller";

        private Dictionary<Policy, TogglePolicy> togglePolicyMap;

        private GroupPolicy()
        {
            this.togglePolicyMap = new Dictionary<Policy, TogglePolicy>();
        }

        /// <summary>
        /// Gets an instance of GroupPolicy.
        /// </summary>
        /// <returns>An instance of type<see cref="GroupPolicy"/> class.</returns>
        public static GroupPolicy GetInstance()
        {
            GroupPolicy groupPolicy = new GroupPolicy();

            try
            {
                List<TogglePolicy> togglePolicies = TogglePolicy.GetAllPolicies();
                groupPolicy.Load(togglePolicies);

                return groupPolicy;
            }
            catch (Exception ex)
            {
                throw new GroupPolicyException(GroupPolicyFailureType.LoadError, ex);
            }
        }

        /// <summary>
        /// Gets the current of status of Policy.
        /// </summary>
        /// <param name="policy">Policy.</param>
        /// <returns>Policy configuration status.</returns>
        public PolicyState GetPolicyState(Policy policy)
        {
            if (!this.togglePolicyMap.ContainsKey(policy))
            {
                throw new GroupPolicyException(policy, GroupPolicyFailureType.NotFound);
            }

            return this.togglePolicyMap[policy].State;
        }

        /// <summary>
        /// Return status of is Policy enabled.
        /// </summary>
        /// <param name="policy">Policy.</param>
        /// <returns>Boolean status indicates is Policy enabled.</returns>
        public bool IsEnabled(Policy policy)
        {
            if (!this.togglePolicyMap.ContainsKey(policy))
            {
                throw new GroupPolicyException(policy, GroupPolicyFailureType.NotFound);
            }

            TogglePolicy togglePolicy = this.togglePolicyMap[policy];

            if (togglePolicy.State == PolicyState.Enabled
                || (togglePolicy.State == PolicyState.NotConfigured && togglePolicy.EnabledByDefault))
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Loads the collection of TogglePolicy.
        /// </summary>
        /// <param name="policies">Enumerable collection of TogglePolicy.</param>
        internal void Load(IEnumerable<TogglePolicy> policies)
        {
            using (RegistryKey regKey = Registry.LocalMachine.OpenSubKey(AppInstallerPolicyRegistryPath))
            {
                foreach (TogglePolicy togglePolicy in policies)
                {
                    // It is likely expected if none of the Windows Package Manager policies are configured i.e Not Configured.
                    if (regKey != null)
                    {
                        var policyValue = regKey.GetValue(togglePolicy.RegistryValueName);

                        RegistryValueKind valueKind = RegistryValueKind.None;

                        if (policyValue != null)
                        {
                            valueKind = regKey.GetValueKind(togglePolicy.RegistryValueName);
                        }

#pragma warning disable CS8604 // Possible null reference argument.
                        togglePolicy.SetValue(policyValue, valueKind);
#pragma warning restore CS8604 // Possible null reference argument.
                    }

                    if (!this.togglePolicyMap.ContainsKey(togglePolicy.PolicyType)
                        || (this.togglePolicyMap.ContainsKey(togglePolicy.PolicyType) && this.togglePolicyMap[togglePolicy.PolicyType] == null))
                    {
                        this.togglePolicyMap.Add(togglePolicy.PolicyType, togglePolicy);
                    }
                }
            }
        }
    }
}
