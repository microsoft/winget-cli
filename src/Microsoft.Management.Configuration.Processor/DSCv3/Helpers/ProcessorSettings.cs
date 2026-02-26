// -----------------------------------------------------------------------------
// <copyright file="ProcessorSettings.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.Helpers;

    /// <summary>
    /// Contains settings for the DSC v3 processor components to share.
    /// </summary>
    internal class ProcessorSettings
    {
        private readonly object dscV3Lock = new ();
        private readonly object defaultPathLock = new ();

        private FindDscPackageStateMachine dscPackageStateMachine = new ();
        private IDSCv3? dscV3 = null;
        private string? defaultPath = null;

        private Dictionary<string, ResourceDetails> resourceDetailsDictionary = new ();

        /// <summary>
        /// Gets or sets the path to the DSC v3 executable.
        /// </summary>
        public string? DscExecutablePath { get; set; }

        /// <summary>
        /// Gets the path to the DSC v3 executable.
        /// </summary>
        public string EffectiveDscExecutablePath
        {
            get
            {
                if (this.DscExecutablePath != null)
                {
                    return this.DscExecutablePath;
                }

                lock (this.defaultPathLock)
                {
                    if (this.defaultPath != null)
                    {
                        return this.defaultPath;
                    }
                }

                string? localDefaultPath = this.GetFoundDscExecutablePath();

                if (localDefaultPath == null)
                {
                    throw new FileNotFoundException("Could not find DSC v3 executable path.");
                }

                lock (this.defaultPathLock)
                {
                    if (this.defaultPath == null)
                    {
                        this.defaultPath = localDefaultPath;
                    }

                    return this.defaultPath;
                }
            }
        }

        /// <summary>
        /// Gets an object for interacting with the DSC executable at EffectiveDscExecutablePath.
        /// </summary>
        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.DocumentationRules", "SA1623:Property summary documentation should match accessors", Justification = "Set is only provided for tests.")]
        public IDSCv3 DSCv3
        {
            get
            {
                lock (this.dscV3Lock)
                {
                    if (this.dscV3 == null)
                    {
                        this.dscV3 = IDSCv3.Create(this);
                    }

                    return this.dscV3;
                }
            }

#if !AICLI_DISABLE_TEST_HOOKS
            set
            {
                lock (this.dscV3Lock)
                {
                    this.dscV3 = value;
                }
            }
#endif
        }

        /// <summary>
        /// Gets or sets the diagnostics sink to use.
        /// </summary>
        public IDiagnosticsSink? DiagnosticsSink { get; set; } = null;

        /// <summary>
        /// Gets or sets a value indicating whether the processor should produce more verbose output.
        /// </summary>
        public bool DiagnosticTraceEnabled { get; set; } = false;

        /// <summary>
        /// Find the DSC v3 executable.
        /// </summary>
        /// <returns>The full path to the dsc.exe executable, or null if not found.</returns>
        public string? GetFoundDscExecutablePath()
        {
            return this.dscPackageStateMachine.DscExecutablePath;
        }

        /// <summary>
        /// Invokes a step in the DSC search state machine.
        /// </summary>
        /// <returns>The transition to take in the state machine.</returns>
        public FindDscPackageStateMachine.Transition PumpFindDscStateMachine()
        {
            return this.dscPackageStateMachine.DetermineNextTransition();
        }

        /// <summary>
        /// Create a deep copy of this settings object.
        /// </summary>
        /// <returns>A deep copy of this object.</returns>
        public ProcessorSettings Clone()
        {
            ProcessorSettings result = new ProcessorSettings();

            result.resourceDetailsDictionary = this.resourceDetailsDictionary;
            result.DiagnosticsSink = this.DiagnosticsSink;
            result.DscExecutablePath = this.DscExecutablePath;
            result.DiagnosticTraceEnabled = this.DiagnosticTraceEnabled;
#if !AICLI_DISABLE_TEST_HOOKS
            result.dscV3 = this.DSCv3;
#endif

            return result;
        }

        /// <summary>
        /// Gets a string representation of this object.
        /// </summary>
        /// <returns>A string representation of this object.</returns>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.Append("EffectiveDscExecutablePath: ");
            sb.AppendLine(this.EffectiveDscExecutablePath);

            sb.Append("DiagnosticTraceLevel: ");
            sb.Append(this.DiagnosticTraceEnabled);

            return sb.ToString();
        }

        /// <summary>
        /// Gets the ResourceDetails for a configuration unit.
        /// </summary>
        /// <param name="configurationUnitInternal">The configuration unit to find details for.</param>
        /// <param name="detailFlags">The level of detail to get.</param>
        /// <returns>The ResourceDetails for the unit, or null if not found.</returns>
        public ResourceDetails? GetResourceDetails(ConfigurationUnitInternal configurationUnitInternal, ConfigurationUnitDetailFlags detailFlags)
        {
            ResourceDetails? result = null;
            bool inDictionary = false;

            lock (this.resourceDetailsDictionary)
            {
                inDictionary = this.resourceDetailsDictionary.TryGetValue(configurationUnitInternal.QualifiedName, out result);
            }

            if (result == null)
            {
                result = new ResourceDetails(configurationUnitInternal.QualifiedName);
            }

            result.EnsureDetails(this, detailFlags);

            if (result.Exists)
            {
                if (!inDictionary)
                {
                    lock (this.resourceDetailsDictionary)
                    {
                        this.resourceDetailsDictionary.Add(configurationUnitInternal.QualifiedName, result);
                    }
                }

                return result;
            }
            else
            {
                return null;
            }
        }

        /// <summary>
        /// Gets all ResourceDetails matching find options.
        /// </summary>
        /// <param name="findOptions">The find options.</param>
        /// <returns>A list of ResourceDetails.</returns>
        public List<ResourceDetails> FindAllResourceDetails(FindUnitProcessorsOptions findOptions)
        {
            List<ResourceDetails> result = new List<ResourceDetails>();

            var resourceItemList = this.DSCv3.GetAllResources(ProcessorRunSettings.CreateFromFindUnitProcessorsOptions(findOptions));

            foreach (var item in resourceItemList)
            {
                ResourceDetails? details = null;
                bool inDictionary = false;
                lock (this.resourceDetailsDictionary)
                {
                    inDictionary = this.resourceDetailsDictionary.TryGetValue(item.Type, out details);
                }

                if (details == null)
                {
                    details = new ResourceDetails(item.Type);
                }

                if (!details.Exists)
                {
                    details.SetResourceListItem(item);
                }

                details.EnsureDetails(this, findOptions.UnitDetailFlags);

                if (!inDictionary)
                {
                    lock (this.resourceDetailsDictionary)
                    {
                        this.resourceDetailsDictionary.Add(item.Type, details);
                    }
                }

                result.Add(details);
            }

            return result;
        }
    }
}
