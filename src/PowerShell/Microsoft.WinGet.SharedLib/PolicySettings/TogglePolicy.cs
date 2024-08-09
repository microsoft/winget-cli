// -----------------------------------------------------------------------------
// <copyright file="TogglePolicy.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------
namespace Microsoft.WinGet.SharedLib.PolicySettings
{
    using System;
    using System.Collections.Generic;

    using Microsoft.Win32;
    using Microsoft.WinGet.SharedLib.Resources;

    /// <summary>
    /// A policyType that acts as a toggle to enable or disable a feature.
    /// They are backed by a DWORD policyValue with values 0 and 1.
    /// </summary>
    public class TogglePolicy
    {
        private readonly Policy policyType;
        private readonly bool isEnabledByDefault;
        private readonly string registryValueName;
        private readonly string resourceString;

        private PolicyState state;
        private int? value;

        /// <summary>
        /// Initializes a new instance of the <see cref="TogglePolicy"/> class.
        /// </summary>
        /// <param name="policy">PolicyType.</param>
        /// <param name="registryValueName">RegistryValue Name.</param>
        /// <param name="policyResourceString">PolicyType ResourceString.</param>
        /// <param name="isEnabledByDefault">Is EnabledByDefault.</param>
        internal TogglePolicy(Policy policy, string registryValueName, string policyResourceString, bool isEnabledByDefault = true)
        {
            this.policyType = policy;
            this.registryValueName = registryValueName;
            this.resourceString = policyResourceString;
            this.isEnabledByDefault = isEnabledByDefault;
            this.state = PolicyState.NotConfigured;
        }

        /// <summary>
        /// Gets PolicyType.
        /// </summary>
        public Policy PolicyType
        {
            get { return this.policyType; }
        }

        /// <summary>
        /// Gets Resource string associated with policy.
        /// </summary>
        public string ResourceString
        {
            get { return this.resourceString; }
        }

        /// <summary>
        /// Gets Policy State.
        /// </summary>
        public PolicyState State
        {
            get { return this.state; }
        }

        /// <summary>
        /// Gets a value indicating whether gets Is policy EnabledByDefault.
        /// </summary>
        internal bool EnabledByDefault
        {
            get { return this.isEnabledByDefault; }
        }

        /// <summary>
        /// Gets Policy RegistryValue Name.
        /// </summary>
        internal string RegistryValueName
        {
            get { return this.registryValueName; }
        }

        /// <summary>
        /// Gets value of the policy.
        /// </summary>
        internal int? Value
        {
            get { return this.value; }
        }

        /// <summary>
        /// Creates an instance of TogglePolicy class.
        /// </summary>
        /// <param name="policy">Policy.</param>
        /// <returns>An instance of <see cref="TogglePolicy"/> class.</returns>
        /// <exception cref="ArgumentException">Argument exception is return if not matching Policy type passed.</exception>
        internal static TogglePolicy Create(Policy policy)
        {
            switch (policy)
            {
                case Policy.WinGet:
                    return new TogglePolicy(policy, "EnableAppInstaller", GroupPolicyResource.PolicyEnableWinGet);
                case Policy.Settings:
                    return new TogglePolicy(policy, "EnableSettings", GroupPolicyResource.PolicyEnableWinGetSettings);
                case Policy.ExperimentalFeatures:
                    return new TogglePolicy(policy, "EnableExperimentalFeatures", GroupPolicyResource.PolicyEnableExperimentalFeatures);
                case Policy.LocalManifestFiles:
                    return new TogglePolicy(policy, "EnableLocalManifestFiles", GroupPolicyResource.PolicyEnableLocalManifests);
                case Policy.HashOverride:
                    return new TogglePolicy(policy, "EnableHashOverride", GroupPolicyResource.PolicyEnableHashOverride);
                case Policy.LocalArchiveMalwareScanOverride:
                    return new TogglePolicy(policy, "EnableLocalArchiveMalwareScanOverride", GroupPolicyResource.PolicyEnableLocalArchiveMalwareScanOverride);
                case Policy.DefaultSource:
                    return new TogglePolicy(policy, "EnableDefaultSource", GroupPolicyResource.PolicyEnableDefaultSource);
                case Policy.MSStoreSource:
                    return new TogglePolicy(policy, "EnableMicrosoftStoreSource", GroupPolicyResource.PolicyEnableMSStoreSource);
                case Policy.AdditionalSources:
                    return new TogglePolicy(policy, "EnableAdditionalSources", GroupPolicyResource.PolicyAdditionalSources);
                case Policy.AllowedSources:
                    return new TogglePolicy(policy, "EnableAllowedSources", GroupPolicyResource.PolicyAllowedSources);
                case Policy.BypassCertificatePinningForMicrosoftStore:
                    return new TogglePolicy(policy, "EnableBypassCertificatePinningForMicrosoftStore", GroupPolicyResource.PolicyEnableBypassCertificatePinningForMicrosoftStore);
                case Policy.WinGetCommandLineInterfaces:
                    return new TogglePolicy(policy, "EnableWindowsPackageManagerCommandLineInterfaces", GroupPolicyResource.PolicyEnableWindowsPackageManagerCommandLineInterfaces);
                case Policy.Configuration:
                    return new TogglePolicy(policy, "EnableWindowsPackageManagerConfiguration", GroupPolicyResource.PolicyEnableWinGetConfiguration);
                default:
                    throw new ArgumentException(null, nameof(policy));
            }
        }

        /// <summary>
        /// Creates a list of TogglePolicy instances.
        /// </summary>
        /// <returns>List of type <see cref="TogglePolicy"/> class.</returns>
        internal static List<TogglePolicy> GetAllPolicies()
        {
            List<TogglePolicy> togglePolicies = new List<TogglePolicy>();

            foreach (Policy policy in Enum.GetValues(typeof(Policy)))
            {
                togglePolicies.Add(Create(policy));
            }

            return togglePolicies;
        }

        /// <summary>
        /// Sets value and state of the policy.
        /// </summary>
        /// <param name="policyValue">PolicyValue object.</param>
        /// <param name="registryValueKind">RegistryValueKind.</param>
        internal void SetValue(object policyValue, RegistryValueKind registryValueKind)
        {
            if (registryValueKind != RegistryValueKind.DWord
                || policyValue == null)
            {
                this.state = PolicyState.NotConfigured;
                this.value = null;

                return;
            }

            this.value = (int)policyValue;

            if (this.value != 0)
            {
                this.state = PolicyState.Enabled;
            }
            else
            {
                this.state = PolicyState.Disabled;
            }
        }
    }
}
