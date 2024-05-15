// -----------------------------------------------------------------------------
// <copyright file="ConfigurationMixedElevationTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.CodeAnalysis;
    using Microsoft.Management.Configuration.Processor.Set;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for verifying the processor behavior for handling mixed elevation scenarios.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    public class ConfigurationMixedElevationTests : ConfigurationProcessorTestBase
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationMixedElevationTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationMixedElevationTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Verifies that a set of units with mixed elevation can run successfully.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ApplyUnitsWithMixedElevation()
        {
            string resourceName = "E2ETestResource";
            string moduleName = "xE2ETestResource";
            Version version = new Version("0.0.0.1");

            ConfigurationSet configurationSet = this.ConfigurationSet();

            ConfigurationUnit configurationUnit1 = this.ConfigurationUnit();
            configurationUnit1.Metadata.Add("securityContext", "elevated");
            configurationUnit1.Metadata.Add("version", version.ToString());
            configurationUnit1.Metadata.Add("module", moduleName);
            configurationUnit1.Metadata.Add("secretCode", "123456789");
            configurationUnit1.Type = resourceName;
            configurationUnit1.Intent = ConfigurationUnitIntent.Apply;

            ConfigurationUnit configurationUnit2 = this.ConfigurationUnit();
            configurationUnit2.Metadata.Add("version", version.ToString());
            configurationUnit2.Metadata.Add("module", moduleName);
            configurationUnit2.Metadata.Add("secretCode", "123456789");
            configurationUnit2.Type = resourceName;
            configurationUnit2.Intent = ConfigurationUnitIntent.Apply;

            configurationSet.Units = new ConfigurationUnit[] { configurationUnit1, configurationUnit2 };

            IConfigurationSetProcessorFactory dynamicFactory = await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync("{73fea39f-6f4a-41c9-ba94-6fd14d633e40}");

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);
            Assert.NotNull(result);
            Assert.Equal(1, result.UnitResults.Count);

            ApplyConfigurationSetResult applyResult = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(applyResult);
        }

        /// <summary>
        /// Verifies that a unit not in the limitation set will fail.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task ApplyUnitNotInLimitationSet()
        {
            string resourceName = "E2ETestResource";
            string moduleName = "xE2ETestResource";
            Version version = new Version("0.0.0.1");

            ConfigurationSet configurationSet = this.ConfigurationSet();

            ConfigurationUnit configurationUnit1 = this.ConfigurationUnit();
            configurationUnit1.Metadata.Add("securityContext", "elevated");
            configurationUnit1.Metadata.Add("version", version.ToString());
            configurationUnit1.Metadata.Add("module", moduleName);
            configurationUnit1.Metadata.Add("secretCode", "123456789");
            configurationUnit1.Type = resourceName;
            configurationUnit1.Intent = ConfigurationUnitIntent.Apply;

            ConfigurationUnit configurationUnit2 = this.ConfigurationUnit();
            configurationUnit2.Metadata.Add("version", version.ToString());
            configurationUnit2.Metadata.Add("module", moduleName);
            configurationUnit2.Metadata.Add("secretCode", "123456789");
            configurationUnit2.Type = resourceName;
            configurationUnit2.Intent = ConfigurationUnitIntent.Apply;

            configurationSet.Units = new ConfigurationUnit[] { configurationUnit1, configurationUnit2 };

            IConfigurationSetProcessorFactory dynamicFactory = await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync("{73fea39f-6f4a-41c9-ba94-6fd14d633e40}");

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);

            TestConfigurationSetResult result = processor.TestSet(configurationSet);
            Assert.NotNull(result);
            Assert.Equal(1, result.UnitResults.Count);

            ApplyConfigurationSetResult applyResult = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(applyResult);
        }
    }
}
