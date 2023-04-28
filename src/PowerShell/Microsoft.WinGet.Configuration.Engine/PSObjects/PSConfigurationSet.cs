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
        /// Getting the details is required.
        /// </summary>
        internal bool HasDetailsRetrieved { get; set; }
    }
}
