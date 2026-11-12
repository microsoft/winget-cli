// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorFactoryTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
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
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory
            {
                ImplicitModulePaths = new List<string> { this.fixture.ExternalModulesPath },
            };

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
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory
            {
                ImplicitModulePaths = new List<string> { this.fixture.ExternalModulesPath },
            };

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
        /// Make sure the context-specific winget path is always added to PSModulePath.
        /// </summary>
        /// <param name="location">Location.</param>
        [Theory]
        [InlineData(PowerShellConfigurationProcessorLocation.CurrentUser)]
        [InlineData(PowerShellConfigurationProcessorLocation.AllUsers)]
        [InlineData(PowerShellConfigurationProcessorLocation.WinGetModulePath)]
        [InlineData(PowerShellConfigurationProcessorLocation.Custom)]
        public void CreateSetProcessor_WinGetPath(PowerShellConfigurationProcessorLocation location)
        {
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory
            {
                ImplicitModulePaths = new List<string> { this.fixture.ExternalModulesPath },
            };

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
            Assert.Contains($"{PowerShellConfigurationSetProcessorFactory.GetDefaultWinGetModulePath().Item1};", modulePath);
        }

        /// <summary>
        /// Full-token discovery excludes inherited ordinary paths, while ordinary discovery remains unchanged.
        /// </summary>
        /// <param name="fullToken">Whether the factory is configuring a full-token environment.</param>
        /// <param name="custom">Whether an explicit custom location is selected.</param>
        [Theory]
        [InlineData(false, false)]
        [InlineData(false, true)]
        [InlineData(true, false)]
        [InlineData(true, true)]
        public void ConfigureModulePaths_TokenContext(bool fullToken, bool custom)
        {
            string ordinaryPath = PowerShellConfigurationSetProcessorFactory.GetUnsecuredDefaultWinGetModulePath();
            string defaultPath = fullToken ? @"C:\Protected\Modules" : ordinaryPath;
            var factory = new PowerShellConfigurationSetProcessorFactory
            {
                AdditionalModulePaths = new List<string> { @"C:\Explicit\Additional" },
                ImplicitModulePaths = new List<string> { @"C:\Explicit\Implicit" },
            };
            if (custom)
            {
                factory.Location = PowerShellConfigurationProcessorLocation.Custom;
                factory.CustomLocation = @"C:\Explicit\Custom";
            }

            var environment = this.fixture.PrepareTestProcessorEnvironment();
            environment.SetPSModulePaths(new List<string>
            {
                @"C:\Other\Modules",
                ordinaryPath.ToUpperInvariant() + @"\",
                Path.Combine(ordinaryPath, "."),
                ordinaryPath.Replace('\\', '/') + "/",
                ordinaryPath + @"\Child",
                ordinaryPath + "Sibling",
            });

            factory.ConfigureModulePaths(environment, fullToken, defaultPath, defaultPath);

            string[] paths = environment.GetVariable<string>(Variables.PSModulePath).Split(';');
            Assert.Contains(defaultPath, paths);
            Assert.Contains(@"C:\Other\Modules", paths);
            Assert.Contains(@"C:\Explicit\Additional", paths);
            Assert.Contains(@"C:\Explicit\Implicit", paths);
            Assert.Contains(ordinaryPath + @"\Child", paths);
            Assert.Contains(ordinaryPath + "Sibling", paths);
            if (fullToken)
            {
                Assert.DoesNotContain(paths, path => string.Equals(
                    Path.GetFullPath(path).TrimEnd('\\', '/'),
                    ordinaryPath,
                    StringComparison.OrdinalIgnoreCase));
            }
            else
            {
                Assert.Contains(ordinaryPath.ToUpperInvariant() + @"\", paths);
            }

            if (custom)
            {
                Assert.Equal(factory.CustomLocation, paths[0]);
            }
        }

        /// <summary>
        /// An explicitly supplied ordinary directory remains authoritative even with a full token.
        /// </summary>
        /// <param name="custom">Whether the ordinary directory is an explicit custom location.</param>
        [Theory]
        [InlineData(false)]
        [InlineData(true)]
        public void ConfigureModulePaths_ExplicitOrdinaryPath(bool custom)
        {
            string ordinaryPath = PowerShellConfigurationSetProcessorFactory.GetUnsecuredDefaultWinGetModulePath();
            var factory = new PowerShellConfigurationSetProcessorFactory();
            if (custom)
            {
                factory.Location = PowerShellConfigurationProcessorLocation.Custom;
                factory.CustomLocation = ordinaryPath;
            }
            else
            {
                factory.AdditionalModulePaths = new List<string> { ordinaryPath };
            }

            var environment = this.fixture.PrepareTestProcessorEnvironment();
            environment.SetPSModulePath(ordinaryPath);
            factory.ConfigureModulePaths(environment, true, @"C:\Protected\Modules", @"C:\Protected\Modules");

            string[] paths = environment.GetVariable<string>(Variables.PSModulePath).Split(';');
            Assert.Contains(ordinaryPath, paths);
            Assert.Contains(@"C:\Protected\Modules", paths);
        }

        /// <summary>
        /// The locally resolved setting supplies the implicit root without changing explicit locations.
        /// </summary>
        /// <param name="explicitLocation">The explicit selection, or null for an implicit settings-based root.</param>
        [Theory]
        [InlineData(null)]
        [InlineData(PowerShellConfigurationProcessorLocation.WinGetModulePath)]
        [InlineData(PowerShellConfigurationProcessorLocation.Custom)]
        public void ConfigureModulePaths_LocallyResolvedRoot(PowerShellConfigurationProcessorLocation? explicitLocation)
        {
            var factory = new PowerShellConfigurationSetProcessorFactory();
            if (explicitLocation.HasValue)
            {
                factory.Location = explicitLocation.Value;
                if (explicitLocation == PowerShellConfigurationProcessorLocation.Custom)
                {
                    factory.CustomLocation = @"C:\Explicit\Modules";
                }
            }

            var environment = this.fixture.PrepareTestProcessorEnvironment();
            environment.SetPSModulePath(@"C:\Other\Modules");
            string effectiveModulePath = explicitLocation.HasValue ? @"C:\Protected\Default" : @"C:\Protected\Configured";
            factory.ConfigureModulePaths(environment, true, @"C:\Protected\Default", effectiveModulePath);

            string[] paths = environment.GetVariable<string>(Variables.PSModulePath).Split(';');
            Assert.Contains(@"C:\Protected\Default", paths);
            string expectedRoot = !explicitLocation.HasValue ? @"C:\Protected\Configured" :
                explicitLocation == PowerShellConfigurationProcessorLocation.Custom ? @"C:\Explicit\Modules" : @"C:\Protected\Default";
            Assert.Equal(expectedRoot, paths[0]);
            if (explicitLocation.HasValue)
            {
                Assert.DoesNotContain(@"C:\Protected\Configured", paths);
            }
        }

        /// <summary>
        /// Tests the custom location is added successfully.
        /// </summary>
        [Fact]
        public void CreateSetProcessor_CustomLocation()
        {
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory
            {
                ImplicitModulePaths = new List<string> { this.fixture.ExternalModulesPath },
            };

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
            var configurationProcessorFactory = new PowerShellConfigurationSetProcessorFactory
            {
                ImplicitModulePaths = new List<string> { this.fixture.ExternalModulesPath },
            };
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
