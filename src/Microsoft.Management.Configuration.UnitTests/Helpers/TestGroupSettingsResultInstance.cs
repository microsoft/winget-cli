// -----------------------------------------------------------------------------
// <copyright file="TestGroupSettingsResultInstance.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System.Collections.Generic;

    /// <summary>
    /// Implements ITestGroupSettingsResult.
    /// </summary>
    internal partial class TestGroupSettingsResultInstance : ITestGroupSettingsResult
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="TestGroupSettingsResultInstance"/> class.
        /// </summary>
        /// <param name="group">The group for this result.</param>
        internal TestGroupSettingsResultInstance(object? group)
        {
            this.Group = group;
        }

        /// <inheritdoc/>
        public object? Group { get; private init; }

        /// <inheritdoc/>
        public IConfigurationUnitResultInformation ResultInformation
        {
            get { return this.InternalResult; }
        }

        /// <summary>
        /// Gets the implementation object for ResultInformation.
        /// </summary>
        public TestConfigurationUnitResultInformation InternalResult { get; } = new TestConfigurationUnitResultInformation();

        /// <inheritdoc/>
        public ConfigurationTestResult TestResult { get; internal set; }

        /// <inheritdoc/>
        public IList<ITestSettingsResult>? UnitResults { get; internal set; }
    }
}
