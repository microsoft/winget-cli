// -----------------------------------------------------------------------------
// <copyright file="OpenConfigurationSetTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for parsing configuration sets from streams.
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

        /// <summary>
        /// Passes an empty stream as input.
        /// </summary>
        [Fact]
        public void EmptyStream()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(string.Empty));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_YAML, result.ResultCode.HResult);
            Assert.Equal(string.Empty, result.Field);
        }

        /// <summary>
        /// Passes a stream with a single null byte in it.
        /// </summary>
        [Fact]
        public void NullByteStream()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream("\0"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_YAML, result.ResultCode.HResult);
            Assert.Equal(string.Empty, result.Field);
        }

        /// <summary>
        /// Passes YAML, but it isn't anything like a configuration file.
        /// </summary>
        [Fact]
        public void NotConfigYAML()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream("yaml: yep"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD, result.ResultCode.HResult);
            Assert.NotEqual(string.Empty, result.Field);
        }

        /// <summary>
        /// Passes YAML without a schema version.
        /// </summary>
        [Fact]
        public void NoConfigVersion()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  thing: 1
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD, result.ResultCode.HResult);
            Assert.NotEqual(string.Empty, result.Field);
        }

        /// <summary>
        /// Passes YAML that appears to be from the distant future.
        /// </summary>
        [Fact]
        public void UnknownConfigVersion()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 99999999
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, result.ResultCode.HResult);
            Assert.Equal("99999999", result.Field);
        }

        /// <summary>
        /// Has one of each type of intent to ensure that it is set properly.
        /// </summary>
        [Fact]
        public void EnsureIntent()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  assertions:
    - resource: Assert
  parameters:
    - resource: Inform
  resources:
    - resource: Apply
"));

            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);
            Assert.Equal(string.Empty, result.Field);

            var units = result.Set.ConfigurationUnits;
            Assert.Equal(3, units.Count);
            bool sawAssert = false;
            bool sawInform = false;
            bool sawApply = false;

            foreach (var unit in units)
            {
                Assert.Equal(unit.UnitName, unit.Intent.ToString());
                switch (unit.Intent)
                {
                    case ConfigurationUnitIntent.Assert: sawAssert = true; break;
                    case ConfigurationUnitIntent.Inform: sawInform = true; break;
                    case ConfigurationUnitIntent.Apply: sawApply = true; break;
                    default: Assert.Fail("Unknown intent"); break;
                }
            }

            Assert.True(sawAssert);
            Assert.True(sawInform);
            Assert.True(sawApply);
        }

        /// <summary>
        /// Passes YAML with resources being something other than a sequence.
        /// </summary>
        [Fact]
        public void NonSequenceUnits()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources: 1
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD, result.ResultCode.HResult);
            Assert.NotEqual(string.Empty, result.Field);
        }

        /// <summary>
        /// Passes YAML with a resource being something other than a map.
        /// </summary>
        [Fact]
        public void NonMapSequenceUnits()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources:
    - string
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD, result.ResultCode.HResult);
            Assert.NotEqual(string.Empty, result.Field);
        }

        /// <summary>
        /// Passes YAML with all values present.
        /// </summary>
        [Fact]
        public void CheckAllUnitProperties()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources:
    - resource: Resource
      id: Identifier
      dependsOn:
        - Dependency1
        - Dependency2
      directives:
        Directive1: A
        Directive2: B
      settings:
        Setting1: '1'
        Setting2: 2
"));
            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);
            Assert.Equal(string.Empty, result.Field);

            Assert.NotEqual(Guid.Empty, result.Set.InstanceIdentifier);

            var units = result.Set.ConfigurationUnits;
            Assert.NotNull(units);
            Assert.Equal(1, units.Count);

            ConfigurationUnit unit = units[0];
            Assert.NotNull(unit);
            Assert.Equal("Resource", unit.UnitName);
            Assert.NotEqual(Guid.Empty, unit.InstanceIdentifier);
            Assert.Equal("Identifier", unit.Identifier);
            Assert.Equal(ConfigurationUnitIntent.Apply, unit.Intent);

            var dependencies = unit.Dependencies;
            Assert.NotNull(dependencies);
            Assert.Equal(2, dependencies.Count);
            Assert.Contains("Dependency1", dependencies);
            Assert.Contains("Dependency2", dependencies);

            var directives = unit.Directives;
            Assert.NotNull(directives);
            Assert.Equal(2, directives.Count);
            Assert.Contains("Directive1", directives);
            Assert.Equal("A", directives["Directive1"]);
            Assert.Contains("Directive2", directives);
            Assert.Equal("B", directives["Directive2"]);

            var settings = unit.Settings;
            Assert.NotNull(settings);
            Assert.Equal(2, settings.Count);
            Assert.Contains("Setting1", settings);
            Assert.Equal("1", settings["Setting1"]);
            Assert.Contains("Setting2", settings);
            Assert.Equal(2L, settings["Setting2"]);

            Assert.Null(unit.Details);
            Assert.Equal(ConfigurationUnitState.Unknown, unit.State);
            Assert.Null(unit.ResultInformation);
            Assert.True(unit.ShouldApply);
        }

        /// <summary>
        /// Test type of scalar nodes.
        /// </summary>
        [Fact]
        public void CheckUnitScalarTypes()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources:
    - resource: Resource
      id: Identifier
      settings:
        SettingInt: 1
        SettingString: '1' 
        SettingBool: false
        SettingStringBool: 'false'
"));
            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);

            var units = result.Set.ConfigurationUnits;
            Assert.NotNull(units);
            Assert.Equal(1, units.Count);

            ConfigurationUnit unit = units[0];
            Assert.NotNull(unit);

            var settings = unit.Settings;
            Assert.NotNull(settings);
            Assert.Equal(4, settings.Count);
            Assert.Contains("SettingInt", settings);
            Assert.Equal(1L, settings["SettingInt"]);
            Assert.Contains("SettingString", settings);
            Assert.Equal("1", settings["SettingString"]);
            Assert.Contains("SettingBool", settings);
            Assert.Equal(false, settings["SettingBool"]);
            Assert.Contains("SettingStringBool", settings);
            Assert.Equal("false", settings["SettingStringBool"]);
        }
    }
}
