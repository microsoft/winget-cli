// -----------------------------------------------------------------------------
// <copyright file="ProcessorSettings.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.Text;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;

    /// <summary>
    /// Contains settings for the DSC v3 processor components to share.
    /// </summary>
    internal class ProcessorSettings
    {
        private readonly object dscV3Lock = new ();
        private IDSCv3? dscV3 = null;

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
                return this.DscExecutablePath ?? throw new NotImplementedException("Determining DSC v3 executable path is not yet implemented");
            }
        }

        /// <summary>
        /// Gets an object for interacting with 
        /// </summary>
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
        }

        /// <summary>
        /// Create a deep copy of this settings object.
        /// </summary>
        /// <returns>A deep copy of this object.</returns>
        public ProcessorSettings Clone()
        {
            // Update if a complex type is added.
            return (ProcessorSettings)this.MemberwiseClone();
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

            return sb.ToString();
        }
    }
}
