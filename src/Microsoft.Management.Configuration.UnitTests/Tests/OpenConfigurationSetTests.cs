// -----------------------------------------------------------------------------
// <copyright file="OpenConfigurationSetTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for configuration set authoring (creating objects).
    /// </summary>
    [Collection("UnitTestCollection")]
    public class OpenConfigurationSetTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="OpenConfigurationSetTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public OpenConfigurationSetTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// Passes a null stream as input.
        /// </summary>
        [Fact]
        public void NullStream()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(null);
            Assert.Null(result.Set);
            Assert.IsType<NullReferenceException>(result.ResultCode);
            Assert.Equal(string.Empty, result.Field);
        }
    }
}
