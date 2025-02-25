// -----------------------------------------------------------------------------
// <copyright file="ProcessorSettings.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System.IO;
    using System.Linq;
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
        /// Find the DSC v3 executable.
        /// </summary>
        /// <returns>The full path to the dsc.exe executable, or null if not found.</returns>
        public static string? FindDscExecutablePath()
        {
            // To start, only attempt to find the package and launch it via the app execution fallback handler.
            // In the future, discover it through %PATH% searching, but probably don't allow that from an elevated process.
            // That probably means creating another read property for finding the secure path.
            Windows.Management.Deployment.PackageManager packageManager = new Windows.Management.Deployment.PackageManager();

            // Until there is a non-preview of this package, use the preview version.
            var packages = packageManager.FindPackagesForUser(null, "Microsoft.DesiredStateConfiguration-Preview_8wekyb3d8bbwe");

            if (packages == null)
            {
                return null;
            }

            string packageInstallLocation = packages.First().InstalledLocation.Path;
            string result = Path.Combine(packageInstallLocation, DscExecutableFileName);

            if (!Path.Exists(result))
            {
                return null;
            }

            return result;
        }

        /// <summary>
        /// Create a deep copy of this settings object.
        /// </summary>
        /// <returns>A deep copy of this object.</returns>
        public ProcessorSettings Clone()
        {
            ProcessorSettings result = new ProcessorSettings();

            result.DscExecutablePath = this.DscExecutablePath;
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
            sb.Append(this.EffectiveDscExecutablePath);

            return sb.ToString();
        }
    }
}
