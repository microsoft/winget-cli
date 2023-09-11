// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetAuthoringTests.cs" company="Microsoft Corporation">
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
    [OutOfProc]
    public class ConfigurationSetAuthoringTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetAuthoringTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationSetAuthoringTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
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

            ConfigurationSet testSet = this.ConfigurationSet();

            testSet.Name = testName;
            Assert.Equal(testName, testSet.Name);
            testSet.Origin = testOrigin;
            Assert.Equal(testOrigin, testSet.Origin);
            testSet.Path = testPath;
            Assert.Equal(testPath, testSet.Path);

            Assert.NotEqual(Guid.Empty, testSet.InstanceIdentifier);
            Assert.Equal(ConfigurationSetState.Unknown, testSet.State);

            Assert.Empty(testSet.Units);
            testSet.Units = new ConfigurationUnit[] { this.ConfigurationUnit() };
            Assert.Equal(1, testSet.Units.Count);

            Assert.NotEqual(string.Empty, testSet.SchemaVersion);
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

            ConfigurationUnit testUnit = this.ConfigurationUnit();

            testUnit.Type = testName;
            Assert.Equal(testName, testUnit.Type);
            testUnit.Identifier = testIdentifier;
            Assert.Equal(testIdentifier, testUnit.Identifier);

            Assert.NotEqual(Guid.Empty, testUnit.InstanceIdentifier);

            Assert.Equal(ConfigurationUnitIntent.Apply, testUnit.Intent);
            testUnit.Intent = testIntent;
            Assert.Equal(testIntent, testUnit.Intent);

            Assert.Empty(testUnit.Dependencies);
            testUnit.Dependencies = new string[] { "dependency1", "dependency2" };
            Assert.Equal(2, testUnit.Dependencies.Count);

            Assert.Empty(testUnit.Metadata);
            Assert.Empty(testUnit.Settings);
            Assert.Null(testUnit.Details);

            Assert.Equal(ConfigurationUnitState.Unknown, testUnit.State);

            Assert.Null(testUnit.ResultInformation);

            Assert.True(testUnit.IsActive);
            testUnit.IsActive = false;
            Assert.False(testUnit.IsActive);
        }

        /// <summary>
        /// This test is to ensure that real tests are added when Serialize is implemented.
        /// </summary>
        [Fact]
        public void ConfigurationSetSerializeNotImplemented()
        {
            Assert.Throws<NotImplementedException>(() => this.ConfigurationSet().Serialize(null));
        }
    }
}
