// -----------------------------------------------------------------------------
// <copyright file="ConfigurationDetailsTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Management.Automation;
    using System.Runtime.InteropServices.WindowsRuntime;
    using Microsoft.Management.Configuration.Processor.Set;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Windows.Security.Cryptography.Certificates;
    using Windows.Storage.Streams;
    using Xunit;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// Tests for ConfigurationUnitProcessorDetails and ConfigurationUnitSettingDetails.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ConfigurationDetailsTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationDetailsTests"/> class.
        /// </summary>
        public ConfigurationDetailsTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test ConfigurationUnitProcessorDetails with invalid args.
        /// </summary>
        [Fact]
        public void ConfigurationUnitProcessorDetails_InvaidArgs()
        {
            Assert.Throws<ArgumentException>(
                () => new ConfigurationUnitProcessorDetails("unitName", null, null, null, null));
        }
    }
}
