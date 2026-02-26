// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorFactoryTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.Management.Configuration.Processor.PowerShell.Set;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using WinRT;
    using Xunit;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.Processor.PowerShell.Constants.PowerShellConstants;

    /// <summary>
    /// Tests ConfigurationProcessorFactory.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
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
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory();

            var properties = configurationProcessorFactory.As<IPowerShellConfigurationProcessorFactoryProperties>();
            properties.ProcessorType = PowerShellConfigurationProcessorType.Hosted;

            var configurationSet = new ConfigurationSet();

            var configurationProcessorSet = configurationProcessorFactory.CreateSetProcessor(configurationSet);

            Assert.NotNull(configurationProcessorSet);
            Assert.IsType<PowerShellConfigurationSetProcessor>(configurationProcessorSet);
            var processorSet = configurationProcessorSet as PowerShellConfigurationSetProcessor;
            Assert.NotNull(processorSet);
        }

        /// <summary>
        /// AdditionalModulePaths test.
        /// </summary>
        [Fact]
        public void CreateSetProcessor_Properties_PsModulePath()
        {
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory();

            var properties = configurationProcessorFactory.As<IPowerShellConfigurationProcessorFactoryProperties>();
            properties.ProcessorType = PowerShellConfigurationProcessorType.Hosted;
            properties.AdditionalModulePaths = new List<string>
                {
                    "ThisIsOnePath",
                    "ThisIsAnotherPath",
                };

            var configurationSet = new ConfigurationSet();

            var configurationProcessorSet = configurationProcessorFactory.CreateSetProcessor(configurationSet);

            Assert.IsType<PowerShellConfigurationSetProcessor>(configurationProcessorSet);
            var processorSet = configurationProcessorSet as PowerShellConfigurationSetProcessor;
            Assert.NotNull(processorSet);

            var modulePath = processorSet.ProcessorEnvironment.GetVariable<string>(Variables.PSModulePath);
            Assert.Contains("ThisIsOnePath;ThisIsAnotherPath", modulePath);
        }

        /// <summary>
        /// Make sure the winget path is always added to PSModulePath.
        /// </summary>
        /// <param name="location">Location.</param>
        [Theory]
        [InlineData(PowerShellConfigurationProcessorLocation.CurrentUser)]
        [InlineData(PowerShellConfigurationProcessorLocation.AllUsers)]
        [InlineData(PowerShellConfigurationProcessorLocation.WinGetModulePath)]
        [InlineData(PowerShellConfigurationProcessorLocation.Custom)]
        public void CreateSetProcessor_WinGetPath(PowerShellConfigurationProcessorLocation location)
        {
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory();

            var properties = configurationProcessorFactory.As<IPowerShellConfigurationProcessorFactoryProperties>();
            properties.Location = location;

            if (properties.Location == PowerShellConfigurationProcessorLocation.Custom)
            {
                properties.CustomLocation = @"c:\this\is\a\module\path";
            }

            var configurationSet = new ConfigurationSet();

            var setProcessor = configurationProcessorFactory.CreateSetProcessor(configurationSet) as PowerShellConfigurationSetProcessor;
            Assert.NotNull(setProcessor);

            var modulePath = setProcessor.ProcessorEnvironment.GetVariable<string>(Variables.PSModulePath);
            Assert.Contains($"{PowerShellConfigurationSetProcessorFactory.GetWinGetModulePath()};", modulePath);
        }

        /// <summary>
        /// Tests the custom location is added successfully.
        /// </summary>
        [Fact]
        public void CreateSetProcessor_CustomLocation()
        {
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory();

            var properties = configurationProcessorFactory.As<IPowerShellConfigurationProcessorFactoryProperties>();
            properties.Location = PowerShellConfigurationProcessorLocation.Custom;
            properties.CustomLocation = @"c:\this\is\a\module\path";

            var configurationSet = new ConfigurationSet();

            var setProcessor = configurationProcessorFactory.CreateSetProcessor(configurationSet) as PowerShellConfigurationSetProcessor;
            Assert.NotNull(setProcessor);

            var modulePath = setProcessor.ProcessorEnvironment.GetVariable<string>(Variables.PSModulePath);
            Assert.Contains($"{properties.CustomLocation};", modulePath);
        }

        /// <summary>
        /// Tests the configuration set processor in limitation mode.
        /// </summary>
        [Fact]
        public void CreateSetProcessor_LimitMode()
        {
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory();
            var configurationSet = new ConfigurationSet();
            configurationProcessorFactory.LimitationSet = configurationSet;

            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.LimitationSet = configurationSet);
            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.ProcessorType = PowerShellConfigurationProcessorType.Default);
            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.AdditionalModulePaths = new List<string>());
            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.ImplicitModulePaths = new List<string>());
            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.Policy = PowerShellConfigurationProcessorPolicy.Unrestricted);
            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.Location = PowerShellConfigurationProcessorLocation.Custom);
            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.CustomLocation = @"c:\this\is\a\module\path");

            var setProcessor = configurationProcessorFactory.CreateSetProcessor(configurationSet) as PowerShellConfigurationSetProcessor;
            Assert.NotNull(setProcessor);
            Assert.True(setProcessor.IsLimitMode);

            // Create processor again in limit mode should fail
            Assert.Throws<System.InvalidOperationException>(() => configurationProcessorFactory.CreateSetProcessor(configurationSet));
        }

        /// <summary>
        /// Tests the configuration set processor factory with ImplicitModulePaths.
        /// </summary>
        [Fact]
        public void ImplicitModulePaths()
        {
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory();

            // When ImplicitModulePaths module paths are not set
            configurationProcessorFactory.AdditionalModulePaths = new List<string> { @"c:\this\is\additional" };
            Assert.Equal(configurationProcessorFactory.AdditionalModulePaths, new List<string> { @"c:\this\is\additional" });

            // Implicit ModulePaths are set, it automatically populates AdditionalModulePaths
            configurationProcessorFactory.ImplicitModulePaths = new List<string> { @"c:\this\is\implicit" };
            Assert.Equal(configurationProcessorFactory.AdditionalModulePaths, new List<string> { @"c:\this\is\additional", @"c:\this\is\implicit" });

            // Set AdditionalModulePaths when ImplicitModulePaths module paths are set
            configurationProcessorFactory.AdditionalModulePaths = new List<string> { @"c:\this\is\additional\2" };
            Assert.Equal(configurationProcessorFactory.AdditionalModulePaths, new List<string> { @"c:\this\is\additional\2", @"c:\this\is\implicit" });
        }
    }
}
