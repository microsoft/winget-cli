// -----------------------------------------------------------------------------
// <copyright file="TestConfigurationUnitResultInformation.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A test implementation of IConfigurationSetProcessorFactory.
    /// </summary>
    internal partial class TestConfigurationUnitResultInformation : IConfigurationUnitResultInformation
    {
        /// <summary>
        /// Gets or sets the description.
        /// </summary>
        public string Description { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the details.
        /// </summary>
        public string Details { get; set; } = string.Empty;

        /// <summary>
        /// Gets or sets the result code.
        /// </summary>
        public Exception? ResultCode { get; set; }

        /// <summary>
        /// Gets or sets the result source.
        /// </summary>
        public ConfigurationUnitResultSource ResultSource { get; set; } = ConfigurationUnitResultSource.None;
    }
}
