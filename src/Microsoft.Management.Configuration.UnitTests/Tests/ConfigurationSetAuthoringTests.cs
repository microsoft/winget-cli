// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetAuthoringTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Windows.Foundation.Collections;
    using Windows.Storage.Streams;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for configuration set authoring (creating objects).
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
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
        /// Basic sanity check to verify that nested value sets can be serialized successfully.
        /// </summary>
        [Fact]
        public void ConfigurationSetSerializeNestedValueSets()
        {
            ConfigurationSet testSet = this.ConfigurationSet();

            testSet.SchemaVersion = "0.2";
            ConfigurationUnit testUnit = this.ConfigurationUnit();
            string testName = "Test Name";
            string testIdentifier = "Test Identifier";
            testUnit.Type = testName;
            testUnit.Identifier = testIdentifier;

            ValueSet innerValueSet = new ValueSet();
            innerValueSet.Add("innerKey", "innerValue");

            ValueSet outerValueSet = new ValueSet();
            outerValueSet.Add("outerKey", innerValueSet);
            testUnit.Metadata = outerValueSet;
            testSet.Units.Add(testUnit);

            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            testSet.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);
            Assert.NotNull(yamlOutput);
        }

        /// <summary>
        /// Test for unique unit evironment calculation.
        /// </summary>
        [Fact]
        public void ConfigurationSet_UnitEnvironments()
        {
            ConfigurationSet testSet = this.ConfigurationSet();

            Dictionary<string, string> firstProperty = new Dictionary<string, string>();
            firstProperty.Add("property", "value1");

            Dictionary<string, string> secondProperty = new Dictionary<string, string>();
            secondProperty.Add("property", "value2");

            Helpers.ConfigurationEnvironmentData[] environments = new Helpers.ConfigurationEnvironmentData[]
            {
                new () { ProcessorIdentifier = "dsc3" },
                new () { ProcessorIdentifier = "pwsh" },
                new () { ProcessorIdentifier = "dsc3", Context = SecurityContext.Elevated },
                new () { ProcessorIdentifier = "pwsh", Context = SecurityContext.Restricted },
                new () { ProcessorIdentifier = "dsc3", ProcessorProperties = firstProperty },
                new () { ProcessorIdentifier = "pwsh", ProcessorProperties = firstProperty },
                new () { ProcessorIdentifier = "pwsh", ProcessorProperties = secondProperty },
                new () { ProcessorIdentifier = "dsc3", Context = SecurityContext.Restricted, ProcessorProperties = firstProperty },
                new () { ProcessorIdentifier = "pwsh", Context = SecurityContext.Elevated, ProcessorProperties = firstProperty },
            };

            foreach (int index in new int[] { 0, 1, 1, 2, 3, 5, 4, 6, 7, 8, 2, 7, 7, 7 })
            {
                Assert.True(index < environments.Length);
                testSet.Units.Add(environments[index].ApplyToUnit(this.ConfigurationUnit()));
            }

            var uniqueEnvironments = testSet.GetUnitEnvironments();

            Assert.Equal(environments.Length, uniqueEnvironments.Count);

            bool[] foundEnvironments = new bool[environments.Length];
            foreach (var actual in uniqueEnvironments)
            {
                for (int i = 0; i < environments.Length; i++)
                {
                    var expected = environments[i];
                    if (actual.Context == expected.Context && actual.ProcessorIdentifier == expected.ProcessorIdentifier && expected.PropertiesEqual(actual.ProcessorPropertiesView))
                    {
                        foundEnvironments[i] = true;
                        break;
                    }
                }
            }

            for (int i = 0; i < foundEnvironments.Length; i++)
            {
                Assert.True(foundEnvironments[i], $"Found expected environment: {i}");
            }
        }
    }
}
