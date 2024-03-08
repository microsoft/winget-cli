// -----------------------------------------------------------------------------
// <copyright file="InstallerSwitches.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.Models.V1
{
    using System;

    /// <summary>
    /// Class that contains different types of installer switches like Custom, Silent and Interactive.
    /// </summary>
    public class InstallerSwitches : IEquatable<InstallerSwitches>
    {
        /// <summary>
        /// Gets or sets the Custom installer switch.
        /// </summary>
        public string Custom { get; set; }

        /// <summary>
        /// Gets or sets the Silent installer switch.
        /// </summary>
        public string Silent { get; set; }

        /// <summary>
        /// Gets or sets the SilentWithProgress installer switch.
        /// </summary>
        public string SilentWithProgress { get; set; }

        /// <summary>
        /// Gets or sets the Interactive installer switch.
        /// </summary>
        public string Interactive { get; set; }

        /// <summary>
        /// Gets or sets the Upgrade installer switch.
        /// </summary>
        public string Upgrade { get; set; }

        /// <summary>
        /// Gets or sets the Log installer switch.
        /// </summary>
        public string Log { get; set; }

        /// <summary>
        /// Gets or sets the install location switch.
        /// </summary>
        public string InstallLocation { get; set; }

        /// <summary>
        /// Gets or sets the repair switch.
        /// </summary>
        public string Repair { get; set; }

        /// <summary>
        /// Override op_Equality.
        /// </summary>
        /// <param name="lhs">left hand side.</param>
        /// <param name="rhs">right hand side.</param>
        /// <returns>boolean indicating if the objects are equals.</returns>
        public static bool operator ==(InstallerSwitches lhs, InstallerSwitches rhs)
        {
            return Equals(lhs, rhs);
        }

        /// <summary>
        /// Override op_Inequality.
        /// </summary>
        /// <param name="lhs">left hand side.</param>
        /// <param name="rhs">right hand side.</param>
        /// <returns>boolean indicating if the objects are not equals.</returns>
        public static bool operator !=(InstallerSwitches lhs, InstallerSwitches rhs)
        {
            return !Equals(lhs, rhs);
        }

        /// <summary>
        /// Override object.Equals.
        /// </summary>
        /// <param name="other">other object.</param>
        /// <returns>boolean indicating if the objects are equals.</returns>
        public override bool Equals(object other)
        {
            return (other is InstallerSwitches) && this.Equals(other as InstallerSwitches);
        }

        /// <summary>
        /// Implemented IEquitable.
        /// </summary>
        /// <param name="other">other object.</param>
        /// <returns>boolean indicating if the objects are equals.</returns>
        public bool Equals(InstallerSwitches other)
        {
            // If parameter is null, return false.
            if (ReferenceEquals(other, null))
            {
                return false;
            }

            // Optimization for a common success case.
            if (ReferenceEquals(this, other))
            {
                return true;
            }

            // If run-time types are not exactly the same, return false.
            if (this.GetType() != other.GetType())
            {
                return false;
            }

            return (this.Custom == other.Custom) &&
                   (this.Silent == other.Silent) &&
                   (this.SilentWithProgress == other.SilentWithProgress) &&
                   (this.Interactive == other.Interactive) &&
                   (this.Upgrade == other.Upgrade) &&
                   (this.Log == other.Log) &&
                   (this.InstallLocation == other.InstallLocation) &&
                   (this.Repair == other.Repair);
        }

        /// <summary>
        /// Override object.GetHashCode. Implement if needed.
        /// to create the hash.
        /// </summary>
        /// <returns>resulting hash.</returns>
        public override int GetHashCode()
        {
            return (this.Custom,
                    this.Silent,
                    this.SilentWithProgress,
                    this.Interactive,
                    this.Upgrade,
                    this.Log,
                    this.InstallLocation,
                    this.Repair).GetHashCode();
        }
    }
}
