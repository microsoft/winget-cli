// -----------------------------------------------------------------------------
// <copyright file="SemanticVersionTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Semantic version tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class SemanticVersionTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="SemanticVersionTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public SemanticVersionTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test version.
        /// </summary>
        [Fact]
        public void SemanticVersion_Test()
        {
            var semanticVersion = new SemanticVersion("1.0.1");
            Assert.Equal("1.0.1", semanticVersion.ToString());
            Assert.Equal(new Version("1.0.1"), semanticVersion.Version);
            Assert.False(semanticVersion.IsPrerelease);
            Assert.Null(semanticVersion.PrereleaseTag);
        }

        /// <summary>
        /// Tests prerelease version.
        /// </summary>
        [Fact]
        public void PrereleaseVersion_Test()
        {
            var semanticVersion = new SemanticVersion("1.0.1-pre");
            Assert.Equal("1.0.1-pre", semanticVersion.ToString());
            Assert.Equal(new Version("1.0.1"), semanticVersion.Version);
            Assert.True(semanticVersion.IsPrerelease);
            Assert.Equal("pre", semanticVersion.PrereleaseTag);
        }

        /// <summary>
        /// Tests GetMaximumVersion.
        /// </summary>
        /// <param name="input">Input.</param>
        /// <param name="expected">Expected.</param>
        [Theory]
        [InlineData("1.0.1", "1.0.1")]
        [InlineData("1.0.*", "1.0.999999999")]
        [InlineData("*.*.1", "999999999.999999999.1")]
        public void MaximumVersion_Test(string input, string expected)
        {
            var semanticVersion = new SemanticVersion(input);
            Assert.Equal(expected, semanticVersion.ToString());
        }
    }
}
