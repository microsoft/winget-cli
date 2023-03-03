// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorFactoryTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.Management.Configuration.Processor.Set;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Moq;
    using Xunit;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// Tests ConfigurationProcessorFactory.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ConfigurationProcessorFactoryTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorFactoryTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorFactoryTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// CreateSetProcessor test.
        /// </summary>
        [Fact]
        public void CreateSetProcessor_Test()
        {
            var configurationProcessorFactory = new ConfigurationSetProcessorFactory(
                ConfigurationProcessorType.Hosted,
                null);

            var configurationSet = new ConfigurationSet();

            var configurationProcessorSet = configurationProcessorFactory.CreateSetProcessor(configurationSet);

            Assert.NotNull(configurationProcessorSet);
            Assert.IsType<ConfigurationSetProcessor>(configurationProcessorSet);
            var processorSet = configurationProcessorSet as ConfigurationSetProcessor;
            Assert.NotNull(processorSet);
        }

        /// <summary>
        /// CreateSetProcessor test.
        /// </summary>
        [Fact]
        public void CreateSetProcessor_Properties_PsModulePath()
        {
            var configurationProcessorFactoryPropertiesMock = new Mock<IConfigurationProcessorFactoryProperties>();
            configurationProcessorFactoryPropertiesMock.Setup(c => c.AdditionalModulePaths)
                .Returns(new List<string>
                {
                    "ThisIsOnePath",
                    "ThisIsAnotherPath",
                });

            var configurationProcessorFactory = new ConfigurationSetProcessorFactory(
                ConfigurationProcessorType.Hosted,
                configurationProcessorFactoryPropertiesMock.Object);

            var configurationSet = new ConfigurationSet();

            var configurationProcessorSet = configurationProcessorFactory.CreateSetProcessor(configurationSet);

            Assert.IsType<ConfigurationSetProcessor>(configurationProcessorSet);
            var processorSet = configurationProcessorSet as ConfigurationSetProcessor;
            Assert.NotNull(processorSet);

            configurationProcessorFactoryPropertiesMock.Verify();

            var modulePath = processorSet.ProcessorEnvironment.GetVariable<string>(Variables.PSModulePath);
            Assert.StartsWith("ThisIsOnePath;ThisIsAnotherPath", modulePath);
        }
    }
}
