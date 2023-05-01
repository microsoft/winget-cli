// -----------------------------------------------------------------------------
// <copyright file="PSConfigurationSet.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.PSObjects
{
    using Microsoft.Management.Configuration;

    /// <summary>
    /// Wrapper for ConfigurationSet.
    /// </summary>
    public sealed class PSConfigurationSet
    {
        private static readonly object ProcessorLock = new ();
        private volatile bool hasDetails = false;
        private volatile bool operationInProgress = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="PSConfigurationSet"/> class.
        /// </summary>
        /// <param name="processor">The configuration processor.</param>
        /// <param name="set">The configuration set.</param>
        internal PSConfigurationSet(ConfigurationProcessor processor, ConfigurationSet set)
        {
            this.Processor = processor;
            this.Set = set;
        }

        /// <summary>
        /// Gets the name.
        /// </summary>
        public string Name
        {
            get
            {
                return this.Set.Name;
            }
        }

        /// <summary>
        /// Gets the origin.
        /// </summary>
        public string Origin
        {
            get
            {
                return this.Set.Origin;
            }
        }

        /// <summary>
        /// Gets the source.
        /// </summary>
        public string Source
        {
            get
            {
                return this.Set.Path;
            }
        }

        /// <summary>
        /// Gets the state.
        /// </summary>
        public string State
        {
            get
            {
                return this.Set.State.ToString();
            }
        }

        /// <summary>
        /// Gets the schema version.
        /// </summary>
        public string SchemaVersion
        {
            get
            {
                return this.Set.SchemaVersion;
            }
        }

        /// <summary>
        /// Gets the ConfigurationProcessor.
        /// </summary>
        internal ConfigurationProcessor Processor { get; private set; }

        /// <summary>
        /// Gets the ConfigurationSet.
        /// </summary>
        internal ConfigurationSet Set { get; private set; }

        /// <summary>
        /// Gets or sets a value indicating whether the details had been retrieved for this set.
        /// </summary>
        internal bool HasDetails
        {
            get
            {
                lock (ProcessorLock)
                {
                    return this.hasDetails;
                }
            }

            set
            {
                lock (ProcessorLock)
                {
                    this.hasDetails = value;
                }
            }
        }

        /// <summary>
        /// Checks if the object is being used by another cmdlet. If not, blocks it for the caller.
        /// </summary>
        /// <returns>True if no one is using me.</returns>
        internal bool CanProcess()
        {
            lock (ProcessorLock)
            {
                if (!this.operationInProgress)
                {
                    this.operationInProgress = true;
                    return true;
                }

                return false;
            }
        }

        /// <summary>
        /// The object is no longer in use by a cmdlet.
        /// </summary>
        internal void DoneProcessing()
        {
            lock (ProcessorLock)
            {
                this.operationInProgress = false;
            }
        }
    }
}
