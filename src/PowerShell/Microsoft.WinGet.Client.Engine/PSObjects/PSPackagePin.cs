// -----------------------------------------------------------------------------
// <copyright file="PSPackagePin.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.PSObjects
{
    using System;
    using Microsoft.Management.Deployment;

    /// <summary>
    /// PSPackagePin wraps a PackagePin COM object for PowerShell output.
    /// </summary>
    public sealed class PSPackagePin
    {
        private readonly PackagePin packagePin;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSPackagePin"/> class.
        /// </summary>
        /// <param name="packagePin">The PackagePin COM object.</param>
        internal PSPackagePin(PackagePin packagePin)
        {
            this.packagePin = packagePin;
        }

        /// <summary>
        /// Gets the package identifier.
        /// </summary>
        public string PackageId
        {
            get { return this.packagePin.PackageId; }
        }

        /// <summary>
        /// Gets the source identifier the pin applies to.
        /// </summary>
        public string SourceId
        {
            get { return this.packagePin.SourceId; }
        }

        /// <summary>
        /// Gets the pin type.
        /// </summary>
        public string Type
        {
            get { return this.packagePin.Type.ToString(); }
        }

        /// <summary>
        /// Gets the gated version range (for Gating pins only).
        /// </summary>
        public string GatedVersion
        {
            get { return this.packagePin.GatedVersion; }
        }

        /// <summary>
        /// Gets the UTC date/time when the pin was added. Null if not set.
        /// </summary>
        public DateTimeOffset? DateAdded
        {
            get { return this.packagePin.DateAdded; }
        }

        /// <summary>
        /// Gets the optional user note for this pin.
        /// </summary>
        public string Note
        {
            get { return this.packagePin.Note; }
        }

        /// <summary>
        /// Gets a value indicating whether this pin applies to an installed package.
        /// </summary>
        public bool IsForInstalledPackage
        {
            get { return this.packagePin.IsForInstalledPackage; }
        }
    }
}
