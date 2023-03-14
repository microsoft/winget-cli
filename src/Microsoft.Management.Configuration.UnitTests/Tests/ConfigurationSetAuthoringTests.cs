// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetAuthoringTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for configuration set authoring (creating objects).
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ConfigurationSetAuthoringTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetAuthoringTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationSetAuthoringTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Creates a configuration set and sets all available properties.
        /// </summary>
        [Fact]
        public void ConfigurationSetAndProperties()
        {
            string testName = "Test Name";
            string testOrigin = "Test Origin";
            string testPath = "TestPath.ext";

            ConfigurationSet testSet = new ConfigurationSet();

            testSet.Name = testName;
            Assert.Equal(testName, testSet.Name);
            testSet.Origin = testOrigin;
            Assert.Equal(testOrigin, testSet.Origin);
            testSet.Path = testPath;
            Assert.Equal(testPath, testSet.Path);

            Assert.NotEqual(Guid.Empty, testSet.InstanceIdentifier);
            Assert.Equal(ConfigurationSetState.Unknown, testSet.State);

            Assert.Empty(testSet.ConfigurationUnits);
            testSet.ConfigurationUnits = new ConfigurationUnit[] { new ConfigurationUnit() };
            Assert.Equal(1, testSet.ConfigurationUnits.Count);
        }

        /// <summary>
        /// Creates a configuration unit and sets all available properties.
        /// </summary>
        [Fact]
        public void ConfigurationUnitAndProperties()
        {
            string testName = "Test Name";
            string testIdentifier = "Test Identifier";
            ConfigurationUnitIntent testIntent = ConfigurationUnitIntent.Assert;

            ConfigurationUnit testUnit = new ConfigurationUnit();

            testUnit.UnitName = testName;
            Assert.Equal(testName, testUnit.UnitName);
            testUnit.Identifier = testIdentifier;
            Assert.Equal(testIdentifier, testUnit.Identifier);

            Assert.NotEqual(Guid.Empty, testUnit.InstanceIdentifier);

            Assert.Equal(ConfigurationUnitIntent.Apply, testUnit.Intent);
            testUnit.Intent = testIntent;
            Assert.Equal(testIntent, testUnit.Intent);

            Assert.Empty(testUnit.Dependencies);
            testUnit.Dependencies = new string[] { "dependency1", "dependency2" };
            Assert.Equal(2, testUnit.Dependencies.Count);

            Assert.Empty(testUnit.Directives);
            Assert.Empty(testUnit.Settings);
            Assert.Null(testUnit.Details);

            Assert.Equal(ConfigurationUnitState.Unknown, testUnit.State);

            Assert.Null(testUnit.ResultInformation);

            Assert.True(testUnit.ShouldApply);
            testUnit.ShouldApply = false;
            Assert.False(testUnit.ShouldApply);
        }

        /// <summary>
        /// This test is to ensure that real tests are added when Serialize is implemented.
        /// </summary>
        [Fact]
        public void ConfigurationSetSerializeNotImplemented()
        {
            Assert.Throws<NotImplementedException>(() => new ConfigurationSet().Serialize(null));
        }
    }
}
