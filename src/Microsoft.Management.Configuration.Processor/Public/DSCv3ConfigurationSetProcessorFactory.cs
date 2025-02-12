// -----------------------------------------------------------------------------
// <copyright file="DSCv3ConfigurationSetProcessorFactory.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor
{
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DSCv3.Set;
    using Microsoft.Management.Configuration.Processor.Factory;
    using System;

    /// <summary>
    /// IConfigurationSetProcessorFactory implementation using DSC v3.
    /// </summary>
    internal sealed partial class DSCv3ConfigurationSetProcessorFactory : ConfigurationSetProcessorFactoryBase, IConfigurationSetProcessorFactory
    {
        private string? dscExecutablePath = null;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ConfigurationSetProcessorFactory"/> class.
        /// </summary>
        public DSCv3ConfigurationSetProcessorFactory()
        {
        }

        /// <summary>
        /// Gets or sets the path to the DSC v3 executable.
        /// </summary>
        public string? DscExecutablePath
        {
            get
            {
                return this.dscExecutablePath;
            }

            set
            {
                if (this.IsLimitMode())
                {
                    throw new InvalidOperationException("Setting DscExecutablePath in limit mode is invalid.");
                }

                this.dscExecutablePath = value;
            }
        }

        /// <inheritdoc />
        protected override IConfigurationSetProcessor CreateSetProcessorInternal(ConfigurationSet? set, bool isLimitMode)
        {
            return new DSCv3ConfigurationSetProcessor(set, isLimitMode);
        }
    }
}
