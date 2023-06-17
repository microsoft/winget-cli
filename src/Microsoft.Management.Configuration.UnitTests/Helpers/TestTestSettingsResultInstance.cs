// -----------------------------------------------------------------------------
// <copyright file="TestTestSettingsResultInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// A test implementation of ITestSettingsResult.
    /// </summary>
    internal class TestTestSettingsResultInstance : ITestSettingsResult
    {
        /// <summary>
        /// Gets the result information.
        /// </summary>
        public IConfigurationUnitResultInformation ResultInformation { get; } = new TestConfigurationUnitResultInformation();

        /// <summary>
        /// Gets or sets the test result.
        /// </summary>
        public ConfigurationTestResult TestResult { get; set; }
    }
}
