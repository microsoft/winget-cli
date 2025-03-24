// -----------------------------------------------------------------------------
// <copyright file="ProcessorSettings.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.IO;
    using System.Text;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// Contains settings for the DSC v3 processor components to share.
    /// </summary>
    internal class ProcessorSettings
    {
        private const string DscExecutableFileName = "dsc.exe";

        private readonly object dscV3Lock = new ();
        private readonly object defaultPathLock = new ();

        private IDSCv3? dscV3 = null;
        private string? defaultPath = null;

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

                string? localDefaultPath = FindDscExecutablePath();

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
        /// Gets or sets a value indicating whether the processor should produce more verbose output.
        /// </summary>
        public bool DiagnosticTraceEnabled { get; set; } = false;

        /// <summary>
        /// Find the DSC v3 executable.
        /// </summary>
        /// <returns>The full path to the dsc.exe executable, or null if not found.</returns>
        public static string? FindDscExecutablePath()
        {
            // To start, only attempt to find the package and launch it via app execution alias.
            // In the future, consider discovering it through %PATH% searching, but probably don't allow that from an elevated process.
            // That probably means creating another read property for finding the secure path.
#if !AICLI_DISABLE_TEST_HOOKS
            string? result = GetDscExecutablePathForPackage("Microsoft.DesiredStateConfiguration-Preview_8wekyb3d8bbwe");
            if (result != null)
            {
                return result;
            }
#endif

            return GetDscExecutablePathForPackage("Microsoft.DesiredStateConfiguration_8wekyb3d8bbwe");
        }

        /// <summary>
        /// Create a deep copy of this settings object.
        /// </summary>
        /// <returns>A deep copy of this object.</returns>
        public ProcessorSettings Clone()
        {
            ProcessorSettings result = new ProcessorSettings();

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

        private static string? GetDscExecutablePathForPackage(string packageFamilyName)
        {
            string localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            string result = Path.Combine(localAppData, "Microsoft\\WindowsApps", packageFamilyName, DscExecutableFileName);

            if (!Path.Exists(result))
            {
                return null;
            }

            return result;
        }
    }
}
